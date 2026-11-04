import json
import socket
import threading
import unittest
import urllib.error
import urllib.request
from http.server import ThreadingHTTPServer

import simulator


class SimulatorTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.obd = simulator.OBDServer(("127.0.0.1", 0), simulator.OBDHandler)
        cls.web = ThreadingHTTPServer(("127.0.0.1", 0), simulator.WebHandler)
        simulator.WEB_PORT = cls.web.server_port
        threading.Thread(target=cls.obd.serve_forever, daemon=True).start()
        threading.Thread(target=cls.web.serve_forever, daemon=True).start()

    @classmethod
    def tearDownClass(cls):
        cls.obd.shutdown(); cls.web.shutdown()
        cls.obd.server_close(); cls.web.server_close()

    def test_obd_communication_and_rpm_changes(self):
        simulator.state.set(900)
        with socket.create_connection(self.obd.server_address, timeout=2) as sock:
            def exchange(command):
                sock.sendall(command + b"\r")
                result = b""
                while b">" not in result:
                    result += sock.recv(256)
                return result
            self.assertIn(b"ELM327", exchange(b"ATZ"))
            self.assertIn(b"OK", exchange(b"ATE0"))
            self.assertIn(b"41 00 00 10 00 00", exchange(b"0100"))
            self.assertIn(b"41 0C 0E 10", exchange(b"010C"))
            simulator.state.set(3000)
            self.assertIn(b"41 0C 2E E0", exchange(b"010C"))
            self.assertIn(b"NO DATA", exchange(b"010D"))

    def test_control_checks_origin_and_range(self):
        base = f"http://127.0.0.1:{self.web.server_port}"
        def set_rpm(value, origin):
            request = urllib.request.Request(base + "/rpm", data=json.dumps({"rpm": value}).encode(),
                headers={"Content-Type": "application/json", "Origin": origin}, method="POST")
            return urllib.request.urlopen(request, timeout=2)
        with set_rpm(6000, base) as response:
            self.assertEqual(response.status, 200)
        self.assertEqual(simulator.state.get(), 6000)
        with self.assertRaises(urllib.error.HTTPError): set_rpm(12001, base)
        with self.assertRaises(urllib.error.HTTPError): set_rpm(1000, "https://example.com")
        self.assertEqual(simulator.state.get(), 6000)


if __name__ == "__main__":
    unittest.main()
