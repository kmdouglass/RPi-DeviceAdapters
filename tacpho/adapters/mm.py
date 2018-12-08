#!/usr/bin/env python3

import argparse
import grp
import logging
import os
import sys

import coloredlogs
import docker

from tacpho.adapters._version import __version__


CONTAINER_USERDATA_FOLDER = "/home/micro-manager/app/userdata"
"""str: The folder from the host that will be mounted into the container.

This value must be sychronized with the ones in the project's Dockerfiles.

"""

DEFAULT_DOCKER_IMAGE = "kmdouglass/rpi-micromanager"
"""str: The docker image name to use if none is provided.

"""

DEFAULT_DOCKER_TAG = "latest"
"""str: The Docker image tag to use if none is provided.

"""

GROUP_DEVICES = {
    "gpio": ["/dev/gpiomem"],
    "video": ["/dev/video{}".format(x) for x in range(8)],
}
"""dict: The mapping of group names to their corresponding peripheral devices.

These values will be filtered before launching a container to remove any devices that are not
currently present on the system.

"""

OUTPUT_DECODING = "utf-8"
"""str: The byte-to-string decoding used for displaying a container's log.

"""

logger = logging.getLogger(__name__)


class DockerError(Exception):
    pass


class GroupNotFoundError(Exception):
    pass


class MMError(Exception):
    """All general errors raise this exception.

    """

    pass


def find_devices(device_mapping):
    """Finds the peripheral devices that are present on the system.

    Parameters
    ----------
    device_mapping : dict
        A dict whose keys are the Unix group names and whose values are lists of the corresponding
        device files.

    Returns
    -------
    dict
        A dict with the same structure as the input device_mapping, but with values that only
        include the device files that are actually present on the system. An empty dict is returned
        if no files are found.

    """
    filtered_mapping = {}
    for group, devices in device_mapping.items():
        device_file_list = [device for device in devices if os.path.exists(device)]
        if device_file_list:
            filtered_mapping[group] = device_file_list

    return filtered_mapping


def get_group_id(name):
    """Returns the UNIX group id for a given name.

    Parameters
    ----------
    name : str
        The name of the UNIX group

    Returns
    -------
    str
        The group ID corresponding to the given name

    """
    try:
        return grp.getgrnam(name).gr_gid
    except KeyError as ex:
        logger.error("The UNIX group '{}' cannot be found on this system.".format(name))
        raise GroupNotFoundError from ex


def cmd_pull(client, args):
    """Pulls the application image from Docker Hub.

    Parameters
    ----------
    client : docker.client.DockerClient
        The Docker API client.
    args : argparse.Namespace
        A Namespace object populated with the values of the command line arguments.

    """
    image = "{image}:{tag}".format(image=args.name, tag=args.tag)
    try:
        logger.info(
            "Pulling {} from Docker Hub. This may take several minutes...".format(image)
        )
        client.images.pull(args.name, args.tag)
        logger.info("Done.")
    except docker.errors.requests.exceptions.HTTPError as ex:
        # Cannot find the image on the server
        logger.error(str(ex))
        raise DockerError from ex
    except Exception as ex:
        logger.error(
            "Failed to pull {} from Docker Hub. Use the debug '-d' flag for more info.".format(
                image
            )
        )
        raise MMError from ex


def cmd_run(client, args):
    """Runs the application container.

    Parameters
    ----------
    client : docker.client.DockerClient
        The Docker API client.
    args : argparse.Namespace
        A Namespace object populated with the values of the command line arguments.

    Returns
    -------
    docker.Container
        The Docker container of the application.

    """
    image = "{image}:{tag}".format(image=args.name, tag=args.tag)

    # Mount the directory it is located in
    _, script = os.path.split(args.script)
    userdata_path = os.path.dirname(os.path.abspath(args.script))
    volume = {userdata_path: {"bind": CONTAINER_USERDATA_FOLDER, "mode": "rw"}}
    logger.info("Will mount directory {} into the container.".format(userdata_path))

    # Find the devices that are present on the system and their Unix group IDs
    device_mapping = find_devices(GROUP_DEVICES)
    group_ids = [get_group_id(group) for group in device_mapping.keys()]
    devices = []
    for device_files in device_mapping.values():
        devices += [
            "{dev}:{dev}:rw".format(dev=device_file) for device_file in device_files
        ]
    logger.debug("Found these group IDs: {}".format(group_ids))
    logger.debug("Found these devices: {}".format(devices))

    config = {
        "detach": True,
        "devices": devices if devices else None,
        "group_add": group_ids if group_ids else None,
        "remove": True,
        "tty": True,
        "volumes": volume,
    }
    try:
        logger.info("Running container {}...".format(image))
        return client.containers.run(image, script, **config)
    except docker.errors.APIError as ex:
        logger.error(str(ex))
        raise DockerError from ex
    except Exception as ex:
        logger.error(
            "Failed to run container {}. Use the debug flag '-d' for more info.".format(
                image
            )
        )
        raise MMError from ex


def parse_cli_args(cli_args):
    """Parses the command line arguments.

    Parameters
    ----------
    cli_args : list of str
        The list of command line arguments passed during script invocation.

    Returns
    -------
    argparse.Namespace
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

    return parser.parse_args(cli_args)


def main():
    args = parse_cli_args(sys.argv[1:])

    fmt = "%(asctime)s %(hostname)s %(funcName)s[%(process)d] %(levelname)s %(message)s"
    coloredlogs.install(level="DEBUG" if args.debug else "INFO", fmt=fmt)

    # Get the Docker client
    client = docker.from_env()

    if hasattr(args, "func"):
        try:
            docker_proc = args.func(client, args)
        except Exception as ex:
            logger.critical("mm.py command failed: {}".format(str(ex)))
            sys.exit(1)

        # Handles multiple return types from the docker-py API
        try:
            # Stream the output logs
            for line in docker_proc.logs(stream=True):
                print(line.decode(OUTPUT_DECODING), end="")
        except AttributeError:
            logger.debug("Return object has no logs to stream.")


if __name__ == "__main__":
    main()
