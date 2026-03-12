# eyetrack C++ library — build automation
# All builds run inside Docker for reproducibility.
# Usage: make [target]   (run from biometric/eyetrack/)

COMPOSE := docker compose
IMAGE   := eyetrack-test
DLIB_MODEL := models/shape_predictor_68_face_landmarks.dat

.PHONY: help test build dev clean rebuild lint test-asan test-tsan count mosquitto models

help: ## Show this help
	@grep -E '^[a-zA-Z_-]+:.*##' $(MAKEFILE_LIST) | \
		awk 'BEGIN {FS = ":.*## "}; {printf "  \033[36m%-12s\033[0m %s\n", $$1, $$2}'

models: $(DLIB_MODEL) ## Download model files

$(DLIB_MODEL):
	@echo "Downloading dlib shape predictor model..."
	@mkdir -p models
	@wget -q "http://dlib.net/files/shape_predictor_68_face_landmarks.dat.bz2" -O /tmp/sp68.dat.bz2
	@bunzip2 -f /tmp/sp68.dat.bz2
	@mv /tmp/sp68.dat $(DLIB_MODEL)
	@echo "Downloaded $(DLIB_MODEL)"

build: models ## Build the Docker image (compile only, no tests)
	$(COMPOSE) build test

test: build ## Build, then run all tests
	$(COMPOSE) run --rm test

dev: models ## Start interactive dev container with source mounted
	$(COMPOSE) run --rm dev

clean: ## Remove docker images and build artifacts
	$(COMPOSE) down --rmi local --remove-orphans 2>/dev/null || true
	docker image rm $(IMAGE) 2>/dev/null || true

rebuild: models ## Full clean rebuild (no cache), then run tests
	$(COMPOSE) build --no-cache test
	$(COMPOSE) run --rm test

test-asan: build ## Run tests under AddressSanitizer + UBSan
	$(COMPOSE) run --rm test-asan

test-tsan: build ## Run tests under ThreadSanitizer
	$(COMPOSE) run --rm test-tsan

lint: ## Run clang-format check (dry-run) inside container
	$(COMPOSE) run --rm dev bash -c '\
		find include src tests -name "*.hpp" -o -name "*.cpp" | \
		xargs clang-format --dry-run --Werror 2>&1'

mosquitto: ## Start Mosquitto MQTT broker
	$(COMPOSE) up -d mosquitto

count: ## Count tests and source lines
	@echo "--- Test count ---"
	@grep -r 'TEST(' tests/unit/ --include='*.cpp' | wc -l | xargs echo "Test cases:"
	@echo ""
	@echo "--- Source lines (excl. tests) ---"
	@find include src -name '*.hpp' -o -name '*.cpp' | xargs wc -l 2>/dev/null | tail -1 || echo "  0 total"
	@echo ""
	@echo "--- Test lines ---"
	@find tests -name '*.cpp' | xargs wc -l 2>/dev/null | tail -1 || echo "  0 total"

# --- Flutter Client ---

FLUTTER := fvm flutter

dev-client:         ## Start Flutter client (web, Chrome)
	cd client && $(FLUTTER) run -d chrome

dev-client-macos:   ## Start Flutter client (macOS native)
	cd client && $(FLUTTER) run -d macos

dev-client-web:   ## Start Flutter client (macOS native)
	cd client && $(FLUTTER) run -d chrome

dev-client-macos-clean:   ## Start Flutter clean client (macOS native)
	cd client && $(FLUTTER) clean && $(FLUTTER) run -d macos

build-client-web:   ## Build Flutter client for web
	cd client && $(FLUTTER) build web --release

build-client-macos: ## Build Flutter client for macOS
	cd client && $(FLUTTER) build macos --release
