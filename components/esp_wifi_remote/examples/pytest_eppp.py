# SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
# SPDX-License-Identifier: CC0-1.0
import os.path
from typing import Tuple

import pytest
from pytest_embedded_idf.dut import IdfDut


@pytest.mark.parametrize(
    'count, app_path, target', [
        (2,
         f'{os.path.join(os.path.dirname(__file__), "mqtt")}|{os.path.join(os.path.dirname(__file__), "server")}',
         'esp32p4|esp32c6'),
    ], indirect=True
)
def test_wifi_remote_eppp(dut: Tuple[IdfDut, IdfDut]) -> None:
    client = dut[0]
    server = dut[1]

    server.expect('rpc_server: Received WIFI event 4', timeout=100)     # wifi station connected
    client.expect('MQTT_EVENT_CONNECTED', timeout=100)
