# Contributing to Samwise Ground Station

Thank you for contributing to the Samwise ground station! This guide will help you set up your development environment and understand our development workflow.

## 🚀 Quick Start

### 1. Clone the Repository
```bash
git clone https://github.com/stanford-ssi/samwise-flight-software.git
cd samwise-flight-software/ground_station
```

### 2. Set Up Development Environment

**Option A: Automatic Setup (Recommended)**
```bash
./setup-dev.sh
```

**Option B: Manual Setup**
```bash
# Install development dependencies
pip3 install -e .[dev]

# Navigate to project root and install pre-commit hooks
cd ..
pre-commit install
cd ground_station
```

**With uv (faster):**
```bash
uv pip install -e .[dev]
cd ..
pre-commit install
cd ground_station
```

---

## 🛠️ Development Tools

### Pre-commit Hooks

Pre-commit hooks automatically run code quality checks before each commit:

**What runs automatically:**
- ✅ **Trailing whitespace removal**
- ✅ **End-of-file fixer**
- ✅ **YAML/JSON/TOML validation**
- ✅ **Merge conflict detection**
- ✅ **Black** - Code formatting
- ✅ **Ruff** - Linting and auto-fixes

**Manual execution:**
```bash
# Run on all files
pre-commit run --all-files

# Run on staged files only
pre-commit run

# Skip hooks for a commit (not recommended)
git commit --no-verify
```

### Code Formatting: Black

Black automatically formats Python code to a consistent style.

```bash
# Format all files
black .

# Check without modifying
black --check .

# See what would change
black --diff .
```

**Configuration:** See `[tool.black]` in `pyproject.toml`
- Line length: 100 characters
- Target: Python 3.8+

### Linting: Ruff

Ruff is an extremely fast Python linter.

```bash
# Lint all files
ruff check .

# Auto-fix issues
ruff check --fix .

# Show only errors
ruff check --select E .
```

**Configuration:** See `[tool.ruff]` in `pyproject.toml`
- Line length: 100 characters
- Rules: Pycodestyle (E, W), Pyflakes (F), Import sorting (I), Naming (N)

---

## 🧪 Testing

### Run Tests
```bash
# All tests
pytest

# Specific test file
pytest tests/test_protocol.py

# With coverage
pytest --cov=. --cov-report=html
open htmlcov/index.html

# Only specific markers
pytest -m unit
pytest -m protocol
pytest -m filtering
```

### Writing Tests

**Test file naming:** `test_*.py`

**Test function naming:** `test_*`

**Use markers:**
```python
import pytest

@pytest.mark.unit
@pytest.mark.protocol
def test_my_feature():
    """Test description"""
    assert my_function() == expected
```

**Available markers:**
- `@pytest.mark.unit` - Unit tests
- `@pytest.mark.integration` - Integration tests
- `@pytest.mark.compatibility` - Platform compatibility
- `@pytest.mark.protocol` - Protocol tests
- `@pytest.mark.filtering` - Filtering tests

---

## 📝 Code Style Guidelines

### Import Organization
```python
# Standard library imports
import os
import sys

# Third-party imports
import pytest
from pydantic import BaseModel

# Local imports
import config
from radio_commands import get_radio
```

**NO relative imports** - Use direct imports for CircuitPython compatibility:
```python
# ✅ Good
import config
from state import state_manager

# ❌ Bad (breaks CircuitPython)
from . import config
from .state import state_manager
```

### Naming Conventions
- **Functions/variables:** `snake_case`
- **Classes:** `PascalCase`
- **Constants:** `UPPER_SNAKE_CASE`
- **Private:** `_leading_underscore`

### Documentation
- Add docstrings to all public functions and classes
- Use type hints where helpful
- Keep line length ≤ 100 characters

---

## 🔄 Git Workflow

### 1. Create a Branch
```bash
git checkout -b feature/your-feature-name
# or
git checkout -b fix/bug-description
```

### 2. Make Changes
```bash
# Edit files
vim my_file.py

# Pre-commit hooks will run automatically on commit
git add .
git commit -m "Add feature: description"
```

### 3. Push and Create PR
```bash
git push origin feature/your-feature-name
```

Then create a Pull Request on GitHub.

### Commit Message Format
```
Add feature: brief description

- Detailed point 1
- Detailed point 2

Resolves #123
```

**Good commit messages:**
- `Add RSSI filtering for beacon packets`
- `Fix callsign verification case sensitivity`
- `Update README with CircuitPython deployment instructions`

**Bad commit messages:**
- `fix bug`
- `updates`
- `WIP`

---

## 🚦 CI/CD Pipeline

GitHub Actions automatically runs on every push and PR:

### Checks that run:
1. **Tests** - Full test suite across Python 3.8-3.12
2. **Compatibility** - Platform-specific tests
3. **Linting** - Black formatting + Ruff linting
4. **Coverage** - Code coverage reporting

**View results:**
- Check the "Actions" tab on GitHub
- PRs show status checks automatically

---

## 📋 Pull Request Checklist

Before submitting a PR, ensure:

- [ ] All tests pass locally (`pytest`)
- [ ] Code is formatted (`black .`)
- [ ] Linting passes (`ruff check .`)
- [ ] Pre-commit hooks pass (`pre-commit run --all-files`)
- [ ] New features have tests
- [ ] Documentation is updated if needed
- [ ] Commit messages are descriptive

---

## 🐛 Troubleshooting

### Pre-commit hooks failing

**Issue:** `black` or `ruff` not found
```bash
# Reinstall dev dependencies
pip3 install -e .[dev]
```

**Issue:** Hooks not running
```bash
# Reinstall hooks
pre-commit install

# Verify installation
pre-commit run --all-files
```

### Import errors in tests

**Issue:** `ModuleNotFoundError`
```bash
# Run pytest from project root, not ground_station/
cd /path/to/samwise-flight-software
pytest ground_station/tests/
```

### Linting errors

**Issue:** Line too long
```bash
# Let black auto-fix it
black your_file.py

# Or split the line manually
```

**Issue:** Import order wrong
```bash
# Ruff can auto-fix import order
ruff check --fix your_file.py
```

---

## 💡 Tips for Contributors

### Fast Development Cycle
```bash
# Watch mode for tests (requires pytest-watch)
pip install pytest-watch
ptw

# Auto-format on save (configure your IDE)
# VSCode: Install Python extension, enable "Format On Save"
# Vim: Use ale or similar with black integration
```

### Debugging Tests
```python
# Add breakpoint in test
import pdb; pdb.set_trace()

# Run single test with output
pytest tests/test_file.py::test_name -v -s
```

### Performance Testing
```python
# Use pytest-benchmark
@pytest.mark.benchmark
def test_performance(benchmark):
    result = benchmark(my_function)
    assert result is not None
```

---

## 📚 Additional Resources

- [Python Type Hints](https://docs.python.org/3/library/typing.html)
- [Pytest Documentation](https://docs.pytest.org/)
- [Black Documentation](https://black.readthedocs.io/)
- [Ruff Documentation](https://docs.astral.sh/ruff/)
- [Pre-commit Documentation](https://pre-commit.com/)

---

## ❓ Questions?

- Open an issue on GitHub
- Ask in the SSI Slack #samwise channel
- Email: ssi@lists.stanford.edu

Happy coding! 🚀
