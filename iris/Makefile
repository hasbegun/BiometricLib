# iris C++ library — build automation
# All builds run inside Docker for reproducibility.
# Usage: make [target]   (run from biometric/iris/)

COMPOSE := docker compose
IMAGE   := iris-test

.PHONY: help test build dev bench clean rebuild lint count test-asan test-tsan visualize fhe-comparison analyze-fhe vv

help: ## Show this help
	@grep -E '^[a-zA-Z_-]+:.*##' $(MAKEFILE_LIST) | \
		awk 'BEGIN {FS = ":.*## "}; {printf "  \033[36m%-12s\033[0m %s\n", $$1, $$2}'

test: ## Build and run all tests (docker compose build test)
	$(COMPOSE) build test

build: ## Build library only (no tests)
	$(COMPOSE) build --build-arg BUILDKIT_INLINE_CACHE=1 test

dev: ## Start interactive dev container with source mounted
	$(COMPOSE) run --rm dev

bench: ## Build and run benchmarks
	$(COMPOSE) build test
	$(COMPOSE) run --rm bench

clean: ## Remove docker images and build artifacts
	$(COMPOSE) down --rmi local --remove-orphans 2>/dev/null || true
	docker image rm $(IMAGE) 2>/dev/null || true

rebuild: ## Full clean rebuild (no cache)
	$(COMPOSE) build --no-cache test

test-asan: ## Run tests under AddressSanitizer + UBSan
	$(COMPOSE) build test
	$(COMPOSE) run --rm test-asan

test-tsan: ## Run tests under ThreadSanitizer
	$(COMPOSE) build test
	$(COMPOSE) run --rm test-tsan

lint: ## Run clang-format check (dry-run) inside container
	$(COMPOSE) run --rm dev bash -c '\
		find include src tests -name "*.hpp" -o -name "*.cpp" | \
		xargs clang-format --dry-run --Werror 2>&1'

comparison: ## Run CASIA1 cross-language comparison
	$(COMPOSE) run --rm comparison

visualize: ## Generate visual verification gallery
	$(COMPOSE) run --rm visualize

fhe-comparison: ## Run FHE vs plaintext matching comparison
	$(COMPOSE) run --rm fhe-comparison
	$(COMPOSE) run --rm analyze-fhe

vv: test comparison visualize fhe-comparison ## Full verification & validation run
	@echo ""
	@echo "V&V complete. See:"
	@echo "  comparison/gallery/           — visual verification images"
	@echo "  comparison/fhe_verification/  — FHE comparison results"

count: ## Count tests and source lines
	@echo "--- Test count ---"
	@grep -r 'TEST(' tests/unit/ --include='*.cpp' | wc -l | xargs echo "Test cases:"
	@echo ""
	@echo "--- Source lines (excl. tests) ---"
	@find include src -name '*.hpp' -o -name '*.cpp' | xargs wc -l | tail -1
	@echo ""
	@echo "--- Test lines ---"
	@find tests -name '*.cpp' | xargs wc -l | tail -1
