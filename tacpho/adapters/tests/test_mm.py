import io
import grp
import logging
import random
import os
import shutil
import string
import sys
import tempfile
import unittest
from unittest.mock import MagicMock, patch

import docker
from tacpho.adapters import mm

logging.disable(logging.ERROR)


class UnitTests(unittest.TestCase):
    def setUp(self):
        # Test data for the command line inputs
        class Args:
            def __init__(self):
                self.name = "test/image"
                self.tag = "tag"
                self.script = "script.py"

        self.args = Args()

        # Test device folder and create test device files
        self.test_dir = tempfile.mkdtemp()
        self.test_devices = {
            "gpio": [os.path.join(self.test_dir, "gpiomem")],
            "video": [
                os.path.join(self.test_dir, "video0"),
                os.path.join(self.test_dir, "video1"),
            ],
        }
        for group, devices in self.test_devices.items():
            for device in devices:
                # Create the empty test device files
                with open(device, "a"):
                    pass

    def tearDown(self):
        shutil.rmtree(self.test_dir)

    def test_find_devices(self):
        device_mapping = {
            "gpio": [os.path.join(self.test_dir, "gpiomem")],
            "video": [
                os.path.join(self.test_dir, "video0"),
                os.path.join(self.test_dir, "video1"),
                os.path.join(self.test_dir, "video2"),
            ],
            "hdd": [os.path.join(self.test_dir, "hdd1")],
        }
        results = mm.find_devices(device_mapping)

        self.assertDictEqual(self.test_devices, results)

    def test_find_devices_no_devices_on_system(self):
        # Delete all files in the test directory
        shutil.rmtree(self.test_dir)
        os.mkdir(self.test_dir)
        results = mm.find_devices(self.test_devices)

        self.assertDictEqual(results, {})

    def test_get_group_id_exits_on_missing_group(self):
        """Get group id should fail to find a non-existent group and exit the script

        """
        # Create a random group name until we find one that doesn't exist
        current_groups = grp.getgrall()
        while True:
            test_group = "".join(
                random.choice(string.ascii_lowercase) for _ in range(8)
            )

            if test_group in (group.gr_name for group in current_groups):
                continue
            else:
                break

        with self.assertRaises(mm.GroupNotFoundError):
            mm.get_group_id(test_group)

    def test_cmd_pull_raises_on_docker_error(self):
        """DockerErrors are properly handled in cmd_pull.

        In this case, a DockerError may be raised when an image is not found on the server.

        """
        client = MagicMock()
        client.images.pull = MagicMock(
            side_effect=docker.errors.requests.exceptions.HTTPError()
        )

        with self.assertRaises(mm.DockerError):
            mm.cmd_pull(client, self.args)

    def test_cmd_pull_raises_on_general_error(self):
        """MMErrors are properly handled in cmd_pull.

        """
        client = MagicMock()
        client.images.pull = MagicMock(side_effect=Exception())

        with self.assertRaises(mm.MMError):
            mm.cmd_pull(client, self.args)

    @patch("tacpho.adapters.mm.get_group_id")
    def test_cmd_run_raises_on_docker_error(self, mock_get_group_id):
        client = MagicMock()
        client.containers.run = MagicMock(
            side_effect=docker.errors.APIError("APIError")
        )
        mock_get_group_id.return_value = "gpio"

        with self.assertRaises(mm.DockerError):
            mm.cmd_run(client, self.args)

    @patch("tacpho.adapters.mm.get_group_id")
    def test_cmd_run_raises_on_general_error(self, mock_get_group_id):
        client = MagicMock()
        client.containers.run = MagicMock(side_effect=Exception())
        mock_get_group_id.return_value = "gpio"

        with self.assertRaises(mm.MMError):
            mm.cmd_run(client, self.args)


class CommandLineInterfaceTests(unittest.TestCase):
    def setUp(self):
        # Capture stdout
        self.stdout = io.StringIO()
        sys.stdout = self.stdout

    def tearDown(self):
        # Reset stdout
        sys.stdout = sys.__stdout__

    def test_cli_debug_short_flag(self):
        test_args = ["-d"]
        args = mm.parse_cli_args(test_args)

        self.assertTrue(args.debug)

    def test_cli_debug_long_flag(self):
        test_args = ["--debug"]
        args = mm.parse_cli_args(test_args)

        self.assertTrue(args.debug)

    def test_cli_version_short_flag(self):
        test_args = ["-v"]

        try:
            mm.parse_cli_args(test_args)
        except SystemExit as ex:
            self.assertEqual(ex.code, 0)

    def test_cli_version_long_flag(self):
        test_args = ["--version"]

        try:
            mm.parse_cli_args(test_args)
        except SystemExit as ex:
            self.assertEqual(ex.code, 0)

    def test_cli_cmd_pull_short_name(self):
        test_args = ["pull", "-n", "test_name"]
        args = mm.parse_cli_args(test_args)

        self.assertEqual(args.name, "test_name")

    def test_cli_cmd_pull_long_name(self):
        test_args = ["pull", "--name", "test_name"]
        args = mm.parse_cli_args(test_args)

        self.assertEqual(args.name, "test_name")

    def test_cli_cmd_pull_short_tag(self):
        test_args = ["pull", "-t", "test_tag"]
        args = mm.parse_cli_args(test_args)

        self.assertEqual(args.tag, "test_tag")

    def test_cli_cmd_pull_long_tag(self):
        test_args = ["pull", "--tag", "test_tag"]
        args = mm.parse_cli_args(test_args)

        self.assertEqual(args.tag, "test_tag")

    def test_cli_cmd_pull_short_name_short_tag(self):
        test_args = ["pull", "-n", "test_name", "-t", "test_tag"]
        args = mm.parse_cli_args(test_args)

        self.assertEqual(args.name, "test_name")
        self.assertEqual(args.tag, "test_tag")

    def test_cli_cmd_pull_long_name_long_tag(self):
        test_args = ["pull", "--name", "test_name", "--tag", "test_tag"]
        args = mm.parse_cli_args(test_args)

        self.assertEqual(args.name, "test_name")
        self.assertEqual(args.tag, "test_tag")

    def test_cli_cmd_run_short_name(self):
        test_args = ["run", "-n", "test_name", "test_script"]
        args = mm.parse_cli_args(test_args)

        self.assertEqual(args.name, "test_name")

    def test_cli_cmd_run_long_name(self):
        test_args = ["run", "--name", "test_name", "test_script"]
        args = mm.parse_cli_args(test_args)

        self.assertEqual(args.name, "test_name")

    def test_cli_cmd_run_short_tag(self):
        test_args = ["run", "-t", "test_tag", "test_script"]
        args = mm.parse_cli_args(test_args)

        self.assertEqual(args.tag, "test_tag")

    def test_cli_cmd_run_long_tag(self):
        test_args = ["run", "--tag", "test_tag", "test_script"]
        args = mm.parse_cli_args(test_args)

        self.assertEqual(args.tag, "test_tag")

    def test_cli_cmd_run_short_short_long_tag(self):
        test_args = ["run", "-n", "test_name", "-t", "test_tag", "test_script"]
        args = mm.parse_cli_args(test_args)

        self.assertEqual(args.name, "test_name")
        self.assertEqual(args.tag, "test_tag")

    def test_cli_cmd_run_long_name_long_tag(self):
        test_args = ["run", "--name", "test_name", "--tag", "test_tag", "test_script"]
        args = mm.parse_cli_args(test_args)

        self.assertEqual(args.name, "test_name")
        self.assertEqual(args.tag, "test_tag")


if __name__ == "__main__":
    unittest.main()
