# Official Wyn Packages

## Available Packages

### 1. AWS SDK
**Package:** `github.com/wyn-lang/aws`
**Version:** 1.0.0

AWS SDK wrapper using AWS CLI.

**Modules:**
- `aws.s3` - S3 operations (list, upload, download)
- `aws.ec2` - EC2 operations (list, start, stop)

**Installation:**
```bash
wyn pkg install github.com/wyn-lang/aws
```

**Usage:**
```wyn
import aws.s3

fn main() {
    const buckets = aws.s3.list_buckets()
    print(buckets)
    
    aws.s3.upload("my-bucket", "file.txt", "/local/file.txt")
}
```

### 2. PostgreSQL Driver
**Package:** `github.com/wyn-lang/postgres`
**Version:** 1.0.0

PostgreSQL database driver using psql CLI.

**Functions:**
- `connect(connection_string)` - Connect to database
- `query(conn, sql)` - Execute query
- `exec(conn, sql)` - Execute statement
- `close(conn)` - Close connection

**Installation:**
```bash
wyn pkg install github.com/wyn-lang/postgres
```

**Usage:**
```wyn
import postgres

fn main() {
    const conn = postgres.connect("postgresql://localhost/mydb")
    const rows = postgres.query(conn, "SELECT * FROM users")
    print(rows)
}
```

### 3. Docker API
**Package:** `github.com/wyn-lang/docker`
**Version:** 1.0.0

Docker API client using docker CLI.

**Functions:**
- `ps()` - List containers
- `images()` - List images
- `run(image, name)` - Run container
- `stop(container)` - Stop container
- `start(container)` - Start container
- `remove(container)` - Remove container
- `logs(container)` - Get logs
- `exec_cmd(container, command)` - Execute command

**Installation:**
```bash
wyn pkg install github.com/wyn-lang/docker
```

**Usage:**
```wyn
import docker

fn main() {
    docker.run("nginx", "my-nginx")
    const logs = docker.logs("my-nginx")
    print(logs)
    docker.stop("my-nginx")
}
```

## Package Locations

Packages are stored in: `/tmp/wyn-packages/`

To publish to GitHub:
1. Create repo: `github.com/wyn-lang/package-name`
2. Push package code
3. Users install with: `wyn pkg install github.com/wyn-lang/package-name`

## Creating Your Own Package

1. Create directory structure:
```
my-package/
├── package.wyn
├── README.md
├── src/
│   └── main.wyn
└── examples/
    └── demo.wyn
```

2. Write package.wyn:
```wyn
package {
    name: "my-package"
    version: "1.0.0"
    author: "your-name"
    description: "Package description"
    license: "MIT"
}
```

3. Write your code in src/

4. Push to GitHub

5. Users install: `wyn pkg install github.com/you/my-package`

## Planned Packages

- [ ] **mysql** - MySQL driver
- [ ] **redis** - Redis client
- [ ] **k8s** - Kubernetes client
- [ ] **terraform** - Terraform wrapper
- [ ] **ansible** - Ansible wrapper
- [ ] **prometheus** - Prometheus client
- [ ] **grafana** - Grafana API

## Contributing

To contribute a package:
1. Create the package
2. Test it works
3. Document it
4. Submit PR to add to this list
