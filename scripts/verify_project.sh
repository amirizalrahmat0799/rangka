#!/usr/bin/env bash
# Builds and tests a generated project the way its own CI would.
#   scripts/verify_project.sh <project-dir> [--docker]
# With --docker it also starts everything with Docker Compose and tries the API.
set -euo pipefail

dir=$(cd "$1" && pwd)
docker=${2:-}

step() { printf '\n\033[1;36m== %s\033[0m\n' "$*"; }

check_part() {
  local d=$1
  if [ -f "$d/pom.xml" ]; then
    step "Spring Boot: mvn verify ($d)"
    (cd "$d" && mvn -B -ntp verify)
  elif [ -f "$d/go.mod" ]; then
    step "Go: vet and test ($d)"
    (cd "$d" && go mod tidy && go vet ./... && go test ./...)
  elif [ -f "$d/requirements.txt" ]; then
    step "FastAPI: pytest ($d)"
    (cd "$d" && python3 -m venv .venv && .venv/bin/pip install -q -r requirements-dev.txt && .venv/bin/python -m pytest -q)
  elif [ -f "$d/CMakeLists.txt" ]; then
    step "C++: build and ctest ($d)"
    # shellcheck disable=SC2086
    (cd "$d" && cmake -B build -DCMAKE_BUILD_TYPE=Release ${CMAKE_ARGS:-} && cmake --build build -j && ctest --test-dir build --output-on-failure)
  elif [ -f "$d/package.json" ]; then
    step "Node: install, typecheck, test, build ($d)"
    (cd "$d" && npm install --no-audit --no-fund && npm run typecheck --if-present && npm test && npm run build)
  else
    echo "nothing to check in $d" >&2
    return 1
  fi
}

if [ -d "$dir/backend" ] || [ -d "$dir/frontend" ]; then
  [ -d "$dir/backend" ] && check_part "$dir/backend"
  [ -d "$dir/frontend" ] && check_part "$dir/frontend"
else
  check_part "$dir"
fi

if [ "$docker" = "--docker" ] && [ -f "$dir/docker-compose.yml" ]; then
  step "Docker Compose"
  cd "$dir"
  trap 'docker compose logs --no-color | tail -100; docker compose down -v' EXIT
  docker compose up -d --build --wait
  base=http://localhost:8080
  [ -d frontend ] && base=http://localhost:3000
  for _ in $(seq 1 60); do curl -sf "$base/api/health" && break; sleep 2; done
  curl -sf -X POST "$base/api/todos" -H "Content-Type: application/json" -d '{"title": "from rangka CI"}'
  curl -sf "$base/api/todos" | grep -q "from rangka CI"
  echo
  trap 'docker compose down -v' EXIT
fi

step "OK: $dir"
