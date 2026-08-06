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

# Check Python version
echo "📋 Checking Python version..."
python_version=$(python3 --version | cut -d' ' -f2 | cut -d'.' -f1,2)
required_version="3.8"

if [ "$(printf '%s\n' "$required_version" "$python_version" | sort -V | head -n1)" != "$required_version" ]; then
    echo "❌ Error: Python $required_version or higher is required. Found: $python_version"
    exit 1
fi
echo "✅ Python $python_version detected"
echo ""

# Install development dependencies
echo "📦 Installing development dependencies..."
python -m pip install -e .[dev]
echo "✅ Dependencies installed"
echo ""

# Navigate to project root for pre-commit
cd ..

# Install pre-commit hooks
echo "🔧 Installing pre-commit hooks..."
pre-commit install
echo "✅ Pre-commit hooks installed"
echo ""

# Run pre-commit on all files to verify setup
echo "🧪 Running pre-commit checks on all files..."
pre-commit run --all-files || echo "⚠️  Some checks failed - this is normal for first run"
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
echo "  • Run tests: pytest"
echo "  • Format code: black ."
echo "  • Lint code: ruff check ."
echo "  • Check hooks: pre-commit run --all-files"
echo ""
echo "💡 Pre-commit hooks will now run automatically before each commit!"
