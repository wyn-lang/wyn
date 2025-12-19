const vscode = require('vscode');
const { LanguageClient } = require('vscode-languageclient/node');

let client;

function activate(context) {
    const serverOptions = {
        command: 'wyn',
        args: ['lsp']
    };

    const clientOptions = {
        documentSelector: [{ scheme: 'file', language: 'wyn' }],
        synchronize: {
            fileEvents: vscode.workspace.createFileSystemWatcher('**/*.wyn')
        }
    };

    client = new LanguageClient(
        'wynLanguageServer',
        'Wyn Language Server',
        serverOptions,
        clientOptions
    );

    client.start();
    
    vscode.window.showInformationMessage('Wyn LSP started!');
}

function deactivate() {
    if (client) {
        return client.stop();
    }
}

module.exports = { activate, deactivate };
