FROM ghcr.io/home-assistant/base:3.22
ARG BUILD_VERSION
ARG BUILD_ARCH
LABEL io.hass.version="${BUILD_VERSION}" io.hass.type="app" io.hass.arch="${BUILD_ARCH}"
RUN apk add --no-cache python3 py3-aiohttp py3-yaml
COPY app /app
WORKDIR /app
CMD ["/command/with-contenv", "python3", "/app/server.py"]
