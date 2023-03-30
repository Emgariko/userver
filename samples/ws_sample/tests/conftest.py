import pytest

from pytest_userver.utils import net

pytest_plugins = ['pytest_userver.plugins.core']

@pytest.fixture(scope='session')
def service_non_http_health_checks(
        service_config_yaml
) -> net.HealthChecks:
    checks = net.get_health_checks_info(service_config_yaml)
    checks.tcp.append(net.HostPort(host='localhost', port=8080))
    return checks
    # /// [service_non_http_health_checker]