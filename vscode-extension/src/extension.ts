import * as path from 'path';
import { workspace, ExtensionContext } from 'vscode';
import {
    LanguageClient,
    LanguageClientOptions,
    ServerOptions,
    TransportKind
} from 'vscode-languageclient/node';

let client: LanguageClient;

export function activate(context: ExtensionContext) {
    // Find wyn executable
    const wynPath = workspace.getConfiguration('wyn').get<string>('path') || 'wyn';
    
    // Server options - start LSP server
    const serverOptions: ServerOptions = {
        command: wynPath,
        args: ['lsp'],
        transport: TransportKind.stdio
    };
    
    // Client options
    const clientOptions: LanguageClientOptions = {
        documentSelector: [{ scheme: 'file', language: 'wyn' }],
        synchronize: {
            fileEvents: workspace.createFileSystemWatcher('**/*.wyn')
        }
    };
    
    // Create and start client
    client = new LanguageClient(
        'wynLanguageServer',
        'Wyn Language Server',
        serverOptions,
        clientOptions
    );
    
    client.start();
}

export function deactivate(): Thenable<void> | undefined {
    if (!client) {
        return undefined;
    }
    return client.stop();
}
