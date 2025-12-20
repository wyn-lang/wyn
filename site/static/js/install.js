// Platform detection and install command switcher for Wyn website

(function() {
  'use strict';

  // Detect user's platform
  function detectPlatform() {
    const platform = navigator.platform.toLowerCase();
    const userAgent = navigator.userAgent.toLowerCase();
    
    if (platform.includes('win') || userAgent.includes('windows')) {
      return 'windows';
    } else if (platform.includes('mac') || platform.includes('darwin')) {
      return 'macos';
    } else if (platform.includes('linux')) {
      return 'linux';
    }
    return 'unix'; // default
  }

  // Get install command for platform
  function getInstallCommand(platform) {
    const commands = {
      windows: 'irm wynlang.com/install.ps1 | iex',
      macos: 'curl -fsSL https://wynlang.com/install.sh | sh',
      linux: 'curl -fsSL https://wynlang.com/install.sh | sh',
      unix: 'curl -fsSL https://wynlang.com/install.sh | sh'
    };
    return commands[platform] || commands.unix;
  }

  // Get platform display name
  function getPlatformName(platform) {
    const names = {
      windows: 'Windows (PowerShell)',
      macos: 'macOS',
      linux: 'Linux',
      unix: 'macOS / Linux'
    };
    return names[platform] || 'Unix';
  }

  // Update install commands on page
  function updateInstallCommands() {
    const platform = detectPlatform();
    const command = getInstallCommand(platform);
    const platformName = getPlatformName(platform);

    // Update all elements with class 'install-command'
    document.querySelectorAll('.install-command').forEach(el => {
      el.textContent = command;
    });

    // Update platform name displays
    document.querySelectorAll('.platform-name').forEach(el => {
      el.textContent = platformName;
    });

    // Set data attribute for styling
    document.body.setAttribute('data-platform', platform);
  }

  // Copy to clipboard function
  window.copyInstallCommand = function(elementId) {
    const element = document.getElementById(elementId) || 
                   document.querySelector('.install-command');
    
    if (!element) return;

    const text = element.textContent;
    
    if (navigator.clipboard && navigator.clipboard.writeText) {
      navigator.clipboard.writeText(text).then(() => {
        showCopyFeedback(element);
      }).catch(err => {
        console.error('Failed to copy:', err);
        fallbackCopy(text);
      });
    } else {
      fallbackCopy(text);
    }
  };

  // Fallback copy method
  function fallbackCopy(text) {
    const textarea = document.createElement('textarea');
    textarea.value = text;
    textarea.style.position = 'fixed';
    textarea.style.opacity = '0';
    document.body.appendChild(textarea);
    textarea.select();
    try {
      document.execCommand('copy');
      showCopyFeedback();
    } catch (err) {
      console.error('Fallback copy failed:', err);
    }
    document.body.removeChild(textarea);
  }

  // Show copy feedback
  function showCopyFeedback(element) {
    const feedback = document.createElement('span');
    feedback.textContent = 'Copied!';
    feedback.className = 'copy-feedback';
    feedback.style.cssText = 'color: #4caf50; margin-left: 10px; font-weight: bold;';
    
    const parent = element?.parentElement || document.body;
    parent.appendChild(feedback);
    
    setTimeout(() => {
      feedback.remove();
    }, 2000);
  }

  // Initialize on DOM ready
  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', updateInstallCommands);
  } else {
    updateInstallCommands();
  }

  // Export for manual use
  window.WynInstaller = {
    detectPlatform,
    getInstallCommand,
    getPlatformName,
    updateInstallCommands
  };
})();
