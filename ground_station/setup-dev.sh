#!/bin/bash
# Development environment setup script for Samwise ground station
# This script installs development dependencies and sets up pre-commit hooks

set -e  # Exit on error

echo "🚀 Setting up Samwise Ground Station development environment..."
echo ""

# Check if we're in the right directory
if [ ! -f "pyproject.toml" ]; then
    echo "❌ Error: pyproject.toml not found. Please run this script from the ground_station directory."
    exit 1
fi

# Check for uv
if ! command -v uv &> /dev/null; then
    echo "❌ Error: uv is not installed. Install it from https://github.com/astral-sh/uv"
    exit 1
fi
echo "✅ uv detected: $(uv --version)"
echo ""

# Install all dependencies (including dev group)
echo "📦 Installing dependencies..."
uv sync
echo "✅ Dependencies installed"
echo ""

# Navigate to project root for pre-commit
cd ..

# Install pre-commit hooks
echo "🔧 Installing pre-commit hooks..."
uv run --project ground_station pre-commit install
echo "✅ Pre-commit hooks installed"
echo ""

# Run pre-commit on all files to verify setup
echo "🧪 Running pre-commit checks on all files..."
uv run --project ground_station pre-commit run --all-files || echo "⚠️  Some checks failed - this is normal for first run"
echo ""

cd ground_station

echo "✨ Development environment setup complete!"
echo ""
echo "📝 What was installed:"
echo "  • pytest & pytest-cov (testing)"
echo "  • black (code formatter)"
echo "  • ruff (linter)"
echo "  • pre-commit (git hooks)"
echo ""
echo "🎯 Next steps:"
echo "  • Run tests: uv run pytest"
echo "  • Format code: uvx --python 3.13 black@26.5.1 ."
echo "  • Lint code: uvx --python 3.13 ruff@0.15.13 check ."
echo "  • Check hooks: uv run pre-commit run --all-files"
echo ""
echo "💡 Pre-commit hooks will now run automatically before each commit!"
