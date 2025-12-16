# Docker API for Wyn

Production-ready Docker container management for Wyn language.

## Installation

```bash
wyn pkg install github.com/wyn-lang/docker
```

## Prerequisites

- Docker installed and running
- Docker Compose (optional, for compose operations)

## Usage

```wyn
import docker

fn main() {
    # Pull and run container
    docker.pull("nginx:latest")
    const container_id = docker.run("nginx:latest", "my-nginx", "-p 80:80")
    
    # Container management
    docker.stop("my-nginx")
    docker.start("my-nginx")
    docker.restart("my-nginx")
    
    # Get container info
    const containers = docker.ps(true)  # Show all containers
    const logs = docker.logs("my-nginx", false)
    
    # Build custom image
    docker.build("./Dockerfile", "my-app:latest", "")
    
    # Network operations
    docker.network_create("my-network", "bridge")
    docker.network_connect("my-network", "my-nginx")
    
    # Cleanup
    docker.remove("my-nginx", true)  # Force remove
    docker.system_prune(false)       # Clean up unused resources
}
```

## Available Functions

### Container Operations
- `run(image: str, name: str, options: str) -> str`
- `run_interactive(image: str, command: str) -> int`
- `stop(container: str) -> bool`
- `start(container: str) -> bool`
- `restart(container: str) -> bool`
- `remove(container: str, force: bool) -> bool`
- `kill(container: str) -> bool`

### Image Operations
- `pull(image: str) -> bool`
- `build(dockerfile_path: str, tag: str, build_args: str) -> bool`
- `push(image: str) -> bool`
- `tag(source: str, target: str) -> bool`
- `rmi(image: str, force: bool) -> bool`

### Information & Monitoring
- `ps(all: bool) -> str`
- `images() -> str`
- `logs(container: str, follow: bool) -> str`
- `inspect(container: str) -> str`
- `stats(container: str) -> str`

### Network Operations
- `network_create(name: str, driver: str) -> bool`
- `network_connect(network: str, container: str) -> bool`
- `network_disconnect(network: str, container: str) -> bool`
- `network_ls() -> str`

### Volume Operations
- `volume_create(name: str) -> bool`
- `volume_rm(name: str) -> bool`
- `volume_ls() -> str`

### Docker Compose
- `compose_up(file: str, detached: bool) -> bool`
- `compose_down(file: str) -> bool`

### System Operations
- `system_prune(all: bool) -> bool`
- `version() -> str`
- `info() -> str`
