# Build with CMake (dependencies are downloaded during configure), then run on a slim image.
FROM debian:bookworm AS build
RUN apt-get update \
 && apt-get install -y --no-install-recommends build-essential cmake ca-certificates{% if postgres %} libpq-dev{% endif %}{% if mysql %} pkg-config default-libmysqlclient-dev{% endif %} \
 && rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY . .
RUN cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j"$(nproc)" && ctest --test-dir build --output-on-failure

FROM debian:bookworm-slim
{% if postgres %}
RUN apt-get update && apt-get install -y --no-install-recommends libpq5 && rm -rf /var/lib/apt/lists/*
{% endif %}
{% if mysql %}
RUN apt-get update && apt-get install -y --no-install-recommends libmariadb3 && rm -rf /var/lib/apt/lists/*
{% endif %}
COPY --from=build /src/build/{% snake %} /usr/local/bin/server
USER nobody
EXPOSE 8080
CMD ["server"]
