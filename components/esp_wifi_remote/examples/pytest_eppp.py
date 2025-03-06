# SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
# SPDX-License-Identifier: CC0-1.0
import os.path
from typing import Tuple

import pytest
from pytest_embedded_idf.dut import IdfDut


@pytest.mark.parametrize(
    'count, app_path, target', [
        (2,
         f'{os.path.join(os.path.dirname(__file__), "server")}|{os.path.join(os.path.dirname(__file__), "mqtt")}',
         'esp32c6|esp32p4'),
    ], indirect=True
)
def test_wifi_remote_eppp(dut: Tuple[IdfDut, IdfDut]) -> None:
    server = dut[0]
    client = dut[1]

    # Check for wifi station connected event (different for hosted and eppp)
    if dut[1].app.sdkconfig.get('ESP_WIFI_REMOTE_LIBRARY_HOSTED') is True:
        server.expect('slave_rpc: Sta mode connected', timeout=100)
    else:
        server.expect('rpc_server: Received WIFI event 4', timeout=100)
    client.expect('MQTT_EVENT_CONNECTED', timeout=100)
