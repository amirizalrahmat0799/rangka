# Build a static binary, then run it in a minimal image that holds nothing else.
FROM golang:{% v_go %}-alpine AS build
WORKDIR /src
COPY . .
{% if db_server %}
# go.sum isn't generated with the project (it records checksums of downloaded modules): tidy creates it
RUN go mod tidy && CGO_ENABLED=0 go build -trimpath -ldflags="-s -w" -o /out/server .
{% else %}
RUN CGO_ENABLED=0 go build -trimpath -ldflags="-s -w" -o /out/server .
{% endif %}

FROM gcr.io/distroless/static-debian12:nonroot
COPY --from=build /out/server /server
EXPOSE 8080
ENTRYPOINT ["/server"]
