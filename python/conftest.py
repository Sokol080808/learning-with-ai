# conftest.py курса Python — служебный файл, трогать не нужно.
#
# Делает property-тесты (hypothesis) ВОСПРОИЗВОДИМЫМИ: derandomize — один и тот же набор
# из сотен примеров на каждом запуске (а не новый случайный), deadline=None — без падений
# по таймауту на медленной машине/CI. Так тест не «мигает»: если он упал, то упадёт снова
# на том же контрпримере, и его можно спокойно чинить.
from hypothesis import HealthCheck, settings

settings.register_profile(
    "course",
    deadline=None,
    derandomize=True,
    suppress_health_check=[HealthCheck.function_scoped_fixture],
)
settings.load_profile("course")
