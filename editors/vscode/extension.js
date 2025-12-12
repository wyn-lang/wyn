const vscode = require('vscode');
const { LanguageClient } = require('vscode-languageclient/node');

let client;

function activate(context) {
    const serverOptions = {
        command: 'wyn-lsp',
        args: []
    };

    const clientOptions = {
        documentSelector: [{ scheme: 'file', language: 'wyn' }]
    };

    client = new LanguageClient(
        'wynLanguageServer',
        'Wyn Language Server',
        serverOptions,
        clientOptions
    );

    client.start();
}

function deactivate() {
    if (client) {
        return client.stop();
    }
}

module.exports = { activate, deactivate };
