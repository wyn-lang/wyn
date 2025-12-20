# Wyn Installation System

Multi-platform installation scripts and strategies for the Wyn programming language.

## Quick Install

### macOS / Linux
```bash
curl -fsSL https://wynlang.com/install.sh | sh
```

### Windows (PowerShell)
```powershell
irm wynlang.com/install.ps1 | iex
```

## Installation Scripts

### `install.sh` - Unix/Linux/macOS Installer

**Features:**
- Auto-detects OS (macOS/Linux) and architecture (ARM64/x86_64)
- Downloads latest release from GitHub
- Installs to `/usr/local/bin`
- Installs standard library to `/usr/local/share/wyn`
- Colored output with progress indicators
- Error handling and validation

**Requirements:**
- `curl` - For downloading
- `tar` - For extraction
- `sudo` - For system-wide installation (if `/usr/local/bin` not writable)

**What it does:**
1. Detects platform (darwin→macos, linux→linux)
2. Detects architecture (x86_64, arm64/aarch64)
3. Fetches latest version from GitHub API
4. Downloads `wyn-{version}-{platform}-{arch}.tar.gz`
5. Extracts to temp directory
6. Copies binary to `/usr/local/bin/wyn`
7. Copies std library to `/usr/local/share/wyn/std`
8. Cleans up temp files

### `install.ps1` - Windows PowerShell Installer

**Features:**
- Auto-detects architecture (x86_64 only, 32-bit not supported)
- Downloads latest release from GitHub
- Installs to `%LOCALAPPDATA%\Wyn`
- Automatically adds to user PATH
- Colored output with progress indicators
- Error handling and validation

**Requirements:**
- PowerShell 5.0+ (Windows 10+)
- `tar` command (built into Windows 10+)
- Internet connection

**What it does:**
1. Detects architecture (64-bit required)
2. Fetches latest version from GitHub API
3. Downloads `wyn-{version}-windows-x86_64.tar.gz`
4. Extracts to `%LOCALAPPDATA%\Wyn`
5. Moves binary to install directory root
6. Adds install directory to user PATH
7. Cleans up temp files

## Website Integration

### Platform Detection (`site/static/js/install.js`)

JavaScript library that:
- Auto-detects user's platform (Windows/macOS/Linux)
- Updates install commands dynamically
- Provides copy-to-clipboard functionality
- Exports `WynInstaller` API for manual use

**Usage in HTML:**
```html
<script src="/js/install.js"></script>

<!-- Auto-updated install command -->
<code class="install-command"></code>

<!-- Platform name -->
<span class="platform-name"></span>

<!-- Copy button -->
<button onclick="copyInstallCommand()">Copy</button>
```

### Install Page (`site/static/install.html`)

Standalone installation page with:
- Platform tabs (macOS/Linux, Windows, Manual)
- Auto-detection on page load
- Copy-to-clipboard buttons
- Direct download links for all platforms
- Manual installation instructions
- Next steps and documentation links

**Access:** `https://wynlang.com/install.html`

## Manual Installation

### From Release Archive

1. **Download** the appropriate archive:
   - macOS ARM64: `wyn-{version}-macos-arm64.tar.gz`
   - macOS x86_64: `wyn-{version}-macos-x86_64.tar.gz`
   - Linux x86_64: `wyn-{version}-linux-x86_64.tar.gz`
   - Linux ARM64: `wyn-{version}-linux-arm64.tar.gz`
   - Windows x86_64: `wyn-{version}-windows-x86_64.tar.gz`

2. **Extract:**
   ```bash
   tar -xzf wyn-*.tar.gz
   ```

3. **Install:**
   
   **Unix/macOS:**
   ```bash
   sudo mv release/wyn /usr/local/bin/
   sudo mkdir -p /usr/local/share/wyn
   sudo cp -r release/std /usr/local/share/wyn/
   ```
   
   **Windows:**
   ```powershell
   # Move to desired location
   Move-Item release C:\Program Files\Wyn
   
   # Add to PATH
   $path = [Environment]::GetEnvironmentVariable("Path", "User")
   [Environment]::SetEnvironmentVariable("Path", "$path;C:\Program Files\Wyn", "User")
   ```

4. **Verify:**
   ```bash
   wyn --version
   ```

## Package Managers (Coming Soon)

### Homebrew (macOS/Linux)
```bash
brew install wyn-lang/tap/wyn
```

### Chocolatey (Windows)
```powershell
choco install wyn
```

### APT (Debian/Ubuntu)
```bash
curl -fsSL https://wynlang.com/gpg.key | sudo apt-key add -
sudo add-apt-repository "deb https://wynlang.com/apt stable main"
sudo apt update
sudo apt install wyn
```

## Uninstallation

### Script-installed (Unix/macOS)
```bash
sudo rm /usr/local/bin/wyn
sudo rm -rf /usr/local/share/wyn
```

### Script-installed (Windows)
```powershell
Remove-Item "$env:LOCALAPPDATA\Wyn" -Recurse -Force

# Remove from PATH manually or:
$path = [Environment]::GetEnvironmentVariable("Path", "User")
$newPath = ($path.Split(';') | Where-Object { $_ -notlike "*Wyn*" }) -join ';'
[Environment]::SetEnvironmentVariable("Path", $newPath, "User")
```

## Testing Installation Scripts

### Local Testing (Unix)
```bash
# Test without installing
bash install.sh --dry-run  # (if implemented)

# Test with local file
bash install.sh
```

### Local Testing (Windows)
```powershell
# Test without installing
.\install.ps1 -WhatIf  # (if implemented)

# Test with local file
.\install.ps1
```

## CI/CD Integration

Installation scripts are automatically tested in GitHub Actions:
- Tested on actual macOS, Linux, and Windows runners
- Verified against latest releases
- Smoke tests run after installation

See `.github/workflows/ci.yml` for details.

## Security Considerations

### Script Verification
Users can inspect scripts before running:
```bash
# View script before running
curl -fsSL https://wynlang.com/install.sh

# Download and inspect
curl -fsSL https://wynlang.com/install.sh -o install.sh
cat install.sh
bash install.sh
```

### HTTPS Only
- All downloads use HTTPS
- GitHub releases are cryptographically signed
- Scripts verify download integrity

### Minimal Permissions
- Unix: Only requires sudo for `/usr/local/bin` (standard practice)
- Windows: Installs to user directory (no admin required)

## Troubleshooting

### "Command not found" after installation

**Unix/macOS:**
- Verify `/usr/local/bin` is in PATH: `echo $PATH`
- Add to PATH: `export PATH="/usr/local/bin:$PATH"`
- Add to shell config: `echo 'export PATH="/usr/local/bin:$PATH"' >> ~/.bashrc`

**Windows:**
- Restart terminal/PowerShell
- Verify PATH: `$env:Path`
- Manually add to PATH via System Properties

### Download fails

- Check internet connection
- Verify GitHub is accessible
- Try manual download from releases page
- Check firewall/proxy settings

### Permission denied

**Unix/macOS:**
- Use `sudo` for installation
- Or install to user directory: `~/bin` or `~/.local/bin`

**Windows:**
- Run PowerShell as Administrator (if needed)
- Or install to user directory (default)

## Contributing

To improve installation scripts:
1. Test on your platform
2. Report issues on GitHub
3. Submit PRs with improvements
4. Add support for new package managers

## Files

- `install.sh` - Unix/Linux/macOS installer
- `install.ps1` - Windows PowerShell installer
- `site/static/js/install.js` - Website platform detection
- `site/static/install.html` - Standalone install page
- `.github/workflows/ci.yml` - CI testing
- `.github/workflows/release.yml` - Release automation

## License

MIT - Same as Wyn language
