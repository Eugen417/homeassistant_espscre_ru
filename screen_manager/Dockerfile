# Official ESPHome distribution: compiler, ESP-IDF, esptool and OTA CLI included.
FROM ghcr.io/esphome/esphome:2026.6.2
ARG BUILD_VERSION
ARG BUILD_ARCH
LABEL io.hass.version="${BUILD_VERSION}" io.hass.type="app" io.hass.arch="${BUILD_ARCH}"
RUN pip install --no-cache-dir 'aiohttp>=3.12,<4' PyYAML
COPY app /app
WORKDIR /app
ENTRYPOINT []
CMD ["python3", "/app/server.py"]
