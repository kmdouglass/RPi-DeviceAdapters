#!/usr/bin/env python3

import argparse
import coloredlogs
import docker
import grp
import logging
import os
import sys
from tacpho.adapters._version import __version__


CONTAINER_USERDATA_FOLDER = "/home/micro-manager/app/userdata"
DEFAULT_DOCKER_IMAGE = "kmdouglass/rpi-micromanager"
DEFAULT_DOCKER_TAG = "latest"
OUTPUT_DECODING = "utf-8"

logger = logging.getLogger(__name__)


def get_group_id(name):
    """Returns the UNIX group id for a given name.

    Parameters
    ----------
    name : str
        The name of the UNIX group

    Returns
    -------
    : str
        The group ID corresponding to the given name

    """
    try:
        return grp.getgrnam(name).gr_gid
    except KeyError:
        logger.error("The UNIX group '{}' cannot be found on this system.".format(name))
        sys.exit(1)


def cmd_pull(args):
    """Pulls the application image from Docker Hub.

    """
    client = docker.from_env()
    image = "{image}:{tag}".format(image=args.name, tag=args.tag)
    try:
        logger.info(
            "Pulling {} from Docker Hub. This may take several minutes...".format(image)
        )
        client.images.pull(args.name, args.tag)
        logger.info("Done.")
    except docker.errors.requests.exceptions.HTTPError as ex:
        logger.error(str(ex))
    except Exception as ex:
        logger.error(
            "Failed to pull {} from Docker Hub. Use the debug '-d' flag for more info.".format(
                image
            )
        )
        logger.debug(str(ex))


def cmd_run(args):
    """Runs the application container.

    Returns
    -------
    : docker.Container
        The Docker container of the application.

    """
    client = docker.from_env()
    image = "{image}:{tag}".format(image=args.name, tag=args.tag)
    script = getattr(args, "script", None)

    # If a script is provided, mount the directory it is located in
    if script:
        userdata_path = os.path.dirname(os.path.abspath(script))
        volume = {userdata_path: {"bind": CONTAINER_USERDATA_FOLDER, "mode": "rw"}}
        logger.info("Will mount directory {} into the container.".format(userdata_path))
    else:
        volume = None

    config = {
        "detach": True,
        "devices": ["/dev/gpiomem:/dev/gpiomem:rw"],
        "group_add": [get_group_id("gpio")],
        "remove": True,
        "tty": True,
        "volumes": volume,
    }
    try:
        logger.info("Running container {}...".format(image))
        return client.containers.run(image, script, **config)
    except docker.errors.APIError as ex:
        logger.error(str(ex))
    except Exception as ex:
        logger.error(
            "Failed to run container {}. Use the debug flag '-d' for more info.".format(
                image
            )
        )
        logger.debug(str(ex))


def parse_cli_args():
    """Parses the command line arguments.

    Returns
    -------
    : argparse.Namespace
        A Namespace object populated with the values of the command line arguments.

    """
    # Main parser
    parser = argparse.ArgumentParser(
        description="Command line interface to the Micro-Manager Python Core wrapper on the "
        "Raspberry Pi."
    )
    parser.add_argument(
        "-d", "--debug", action="store_true", help="print debugging information"
    )
    parser.add_argument(
        "-v", "--version", action="version", version="%(prog)s {}".format(__version__)
    )
    subparsers = parser.add_subparsers()

    # ----- Parent parser to the subparsers -----
    parent_parser = argparse.ArgumentParser(add_help=False)
    parent_parser.add_argument(
        "-n",
        "--name",
        type=str,
        default=DEFAULT_DOCKER_IMAGE,
        help="the name of the image (default: {})".format(DEFAULT_DOCKER_IMAGE),
    )
    parent_parser.add_argument(
        "-t",
        "--tag",
        type=str,
        default=DEFAULT_DOCKER_TAG,
        help="the tag of the image (default: {})".format(DEFAULT_DOCKER_TAG),
    )

    # ----- Pull subparser -----
    subparser_pull = subparsers.add_parser(
        "pull", help="download an image of the application", parents=[parent_parser]
    )
    subparser_pull.set_defaults(func=cmd_pull)

    # ----- Run subparser -----
    subparser_run = subparsers.add_parser(
        "run",
        help="run the Micro-Manager application container",
        parents=[parent_parser],
    )
    subparser_run.add_argument(
        "script",
        type=str,
        help="a Python script to execute; the folder that contains this script will be mounted "
        "into the container and may be used for persistent data storage",
    )
    subparser_run.set_defaults(func=cmd_run)

    return parser.parse_args()


def main():
    args = parse_cli_args()

    fmt = "%(asctime)s %(hostname)s %(funcName)s[%(process)d] %(levelname)s %(message)s"
    coloredlogs.install(level="DEBUG" if args.debug else "INFO", fmt=fmt)

    if hasattr(args, "func"):
        docker_proc = args.func(args)

        # Handles multiple return types from the docker-py API
        try:
            # Stream the output logs
            for line in docker_proc.logs(stream=True):
                print(line.decode(OUTPUT_DECODING), end="")
        except AttributeError:
            logger.debug("Return object has no logs to stream.")


if __name__ == "__main__":
    main()
