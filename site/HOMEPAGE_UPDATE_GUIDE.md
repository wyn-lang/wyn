# Homepage Update Guide

## How to Integrate Installation System into Homepage

### 1. Add JavaScript to Layout

In your Hugo layout file (e.g., `layouts/index.html` or `themes/.../layouts/index.html`):

```html
<head>
  <!-- Existing head content -->
  <script src="/js/install.js"></script>
</head>
```

### 2. Update Hero Section

Replace the current "Get Started" section with:

```html
<div class="hero-install">
  <h2>Get Started in 30 Seconds</h2>
  
  <!-- Auto-detected install command -->
  <div class="install-command-box">
    <code class="install-command">curl -fsSL https://wynlang.com/install.sh | sh</code>
    <button onclick="copyInstallCommand()" class="copy-btn">Copy</button>
  </div>
  
  <!-- Platform indicator -->
  <p class="platform-note">
    Detected: <span class="platform-name">macOS / Linux</span>
    • <a href="/install.html">Other platforms</a>
  </p>
  
  <!-- Alternative options -->
  <div class="install-alternatives">
    <a href="/install.html">Installation Guide</a>
    <span>•</span>
    <a href="https://github.com/wyn-lang/wyn/releases">Download Releases</a>
    <span>•</span>
    <a href="/learn">Documentation</a>
  </div>
</div>
```

### 3. Add CSS Styling

```css
.install-command-box {
  background: #1e1e1e;
  color: #d4d4d4;
  padding: 15px 20px;
  border-radius: 8px;
  font-family: 'Monaco', 'Courier New', monospace;
  font-size: 14px;
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin: 20px 0;
  box-shadow: 0 4px 6px rgba(0,0,0,0.1);
}

.install-command {
  flex: 1;
  user-select: all;
}

.copy-btn {
  background: #007acc;
  color: white;
  border: none;
  padding: 8px 16px;
  border-radius: 4px;
  cursor: pointer;
  font-size: 13px;
  margin-left: 15px;
  transition: background 0.2s;
}

.copy-btn:hover {
  background: #005a9e;
}

.platform-note {
  text-align: center;
  color: #666;
  font-size: 14px;
  margin-top: 10px;
}

.platform-name {
  font-weight: bold;
  color: #007acc;
}

.install-alternatives {
  text-align: center;
  margin-top: 20px;
  font-size: 14px;
}

.install-alternatives a {
  color: #007acc;
  text-decoration: none;
}

.install-alternatives a:hover {
  text-decoration: underline;
}
```

### 4. Quick Start Section

Add below the hero:

```html
<section class="quick-start">
  <div class="container">
    <h2>Three Steps to Your First Program</h2>
    
    <div class="steps-grid">
      <div class="step">
        <div class="step-number">1</div>
        <h3>Install</h3>
        <div class="step-code">
          <code class="install-command"></code>
        </div>
      </div>
      
      <div class="step">
        <div class="step-number">2</div>
        <h3>Write</h3>
        <div class="step-code">
          <code>echo 'fn main() { print("Hello!") }' > hello.wyn</code>
        </div>
      </div>
      
      <div class="step">
        <div class="step-number">3</div>
        <h3>Run</h3>
        <div class="step-code">
          <code>wyn run hello.wyn</code>
        </div>
      </div>
    </div>
  </div>
</section>
```

## Example: Minimal Homepage Update

If you want the absolute minimal change to the current homepage:

### Option A: Just add the script and update one line

```html
<!-- Add to <head> -->
<script src="/js/install.js"></script>

<!-- Replace current install command with: -->
<code class="install-command">curl -fsSL https://wynlang.com/install.sh | sh</code>
<button onclick="copyInstallCommand()">Copy</button>
<p><small>Windows: <a href="/install.html">See install guide</a></small></p>
```

That's it! The JavaScript will auto-update the command based on platform.

### Option B: Full featured (recommended)

Use the complete hero section from above with:
- Auto-detection
- Platform tabs
- Copy buttons
- Alternative links

## Testing

### Test install.sh locally
```bash
# Dry run (view what it would do)
bash -x install.sh

# Test with local file
bash install.sh
```

### Test install.ps1 locally
```powershell
# View script
Get-Content install.ps1

# Test (will actually install)
.\install.ps1
```

### Test website integration
1. Open `site/static/install.html` in browser
2. Check platform auto-detection
3. Test copy buttons
4. Try switching platforms manually

## Deployment Checklist

- [x] install.sh created and executable
- [x] install.ps1 created
- [x] install.js created with platform detection
- [x] install.html created with full UI
- [x] INSTALLATION.md documentation created
- [ ] Update homepage to include install.js
- [ ] Add link to /install.html in navigation
- [ ] Test scripts on actual platforms
- [ ] Update wynlang.com DNS/hosting to serve scripts

## Professional Touch

The installation system now matches industry standards:
- **Rust**: `curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh`
- **Node**: `curl -fsSL https://deb.nodesource.com/setup_lts.x | sudo -E bash -`
- **Deno**: `curl -fsSL https://deno.land/install.sh | sh`
- **Wyn**: `curl -fsSL https://wynlang.com/install.sh | sh` ✅

Clean, professional, and developer-friendly!
