# Production DevOps Examples

Real-world examples demonstrating Wyn's power for infrastructure automation, SRE work, and DevOps tasks.

## Why These Examples Matter

These aren't toy examples - they're **production-grade scripts** that solve real problems DevOps engineers face daily. Each example demonstrates why Wyn is better than bash/Python/PowerShell:

- ✅ **Type safety** - Catch errors at compile time
- ✅ **Error handling** - Explicit Result types, no silent failures
- ✅ **Concurrency** - Parallel execution with spawn blocks
- ✅ **Clean syntax** - Readable, maintainable code
- ✅ **Fast execution** - Compiled, not interpreted
- ✅ **Single binary** - No dependency hell

## Examples

### 1. AWS Lambda Deployment (`aws_deploy.wyn`)

**Problem:** Deploying Lambda functions with bash is error-prone and lacks proper error handling.

**Solution:** Type-safe deployment with validation, health checks, and smoke tests.

**Features:**
- Credential validation
- Environment variable updates
- Wait for function to become active
- Automatic smoke testing
- Proper error handling with Result types

**Usage:**
```bash
export AWS_PROFILE=production
export FUNCTION_NAME=my-api
export ZIP_FILE=function.zip
./aws_deploy.wyn
```

**Replaces:** 200+ lines of bash with 100 lines of clean, type-safe Wyn code.

### 2. Docker Health Monitoring (`docker_healthcheck.wyn`)

**Problem:** Container health checks in bash are fragile and don't handle errors well.

**Solution:** Robust health monitoring with automatic restart and alerting.

**Features:**
- Concurrent health checks for multiple containers
- Docker health status inspection
- HTTP endpoint verification
- Automatic container restart
- Alert integration (Slack/PagerDuty ready)

**Usage:**
```bash
./docker_healthcheck.wyn
```

**Replaces:** Complex bash scripts with proper error handling and concurrency.

### 3. Log Analysis (`log_analyzer.wyn`)

**Problem:** Python log analysis is slow, bash is unreadable.

**Solution:** Fast, concurrent log analysis with intelligent alerting.

**Features:**
- Regex pattern matching
- Error counting by service
- Threshold-based alerting
- Log archival and rotation
- 10x faster than Python

**Usage:**
```bash
export LOG_FILE=/var/log/app.log
export ERROR_THRESHOLD=100
./log_analyzer.wyn
```

**Replaces:** Slow Python scripts or cryptic bash one-liners.

### 4. Infrastructure Health Check (`infra_healthcheck.wyn`)

**Problem:** Monitoring multiple services requires complex orchestration.

**Solution:** Concurrent health checks with intelligent error handling.

**Features:**
- Parallel HTTP endpoint checks
- Database connectivity tests
- System resource monitoring (CPU, memory, disk)
- Network connection tracking
- Comprehensive reporting

**Usage:**
```bash
./infra_healthcheck.wyn
```

**Replaces:** Multiple monitoring scripts with one clean, concurrent solution.

### 5. Database Backup (`db_backup.wyn`)

**Problem:** Backup scripts are critical but often fragile.

**Solution:** Production-grade backup with verification and S3 upload.

**Features:**
- PostgreSQL backup creation
- Backup verification
- Compression
- S3 upload
- Automatic rotation
- Complete error handling

**Usage:**
```bash
export DB_NAME=production_db
export S3_BUCKET=my-backups
export KEEP_LOCAL=7
./db_backup.wyn
```

**Replaces:** Fragile bash scripts with proper error handling and validation.

### 6. Kubernetes Deployment (`k8s_deploy.wyn`)

**Problem:** K8s deployments need validation and rollback capabilities.

**Solution:** Safe deployments with automatic rollback on failure.

**Features:**
- Image update
- Rollout monitoring
- Pod health verification
- Automatic rollback on failure
- State backup
- Zero-downtime deployments

**Usage:**
```bash
export DEPLOYMENT=my-api
export NAMESPACE=production
export IMAGE=myapp:v2.0.0
./k8s_deploy.wyn
```

**Replaces:** Complex deployment scripts with safe, tested automation.

### 7. CI/CD Pipeline (`cicd_pipeline.wyn`)

**Problem:** CI/CD pipelines in bash are hard to maintain.

**Solution:** Complete pipeline with proper stages and error handling.

**Features:**
- Test execution
- Docker image building
- Registry push
- Staging deployment
- Production deployment (branch-gated)
- Smoke testing
- Slack notifications

**Usage:**
```bash
export REGISTRY=docker.io/mycompany
export AUTO_APPROVE=true
./cicd_pipeline.wyn
```

**Replaces:** Jenkins/GitLab CI scripts with clean, testable code.

### 8. Incident Response (`incident_response.wyn`)

**Problem:** Manual incident response is slow and error-prone.

**Solution:** Automated detection and remediation.

**Features:**
- OOM killer detection
- Disk full detection
- High error rate detection
- Automatic remediation
- Service restart
- Disk cleanup
- Incident reporting

**Usage:**
```bash
./incident_response.wyn
```

**Replaces:** Manual runbooks with automated, tested responses.

### 9. Secrets Rotation (`secrets_rotation.wyn`)

**Problem:** Manual secret rotation is risky and time-consuming.

**Solution:** Automated rotation with zero downtime.

**Features:**
- Secure secret generation
- Vault integration
- Kubernetes secret updates
- Rolling pod restarts
- Health verification
- Rollback on failure

**Usage:**
```bash
export DEPLOYMENT=my-api
export NAMESPACE=production
./secrets_rotation.wyn
```

**Replaces:** Manual, error-prone secret rotation procedures.

### 10. Cloud Inventory (`cloud_inventory.wyn`)

**Problem:** Tracking resources across multiple clouds is tedious.

**Solution:** Automated discovery and reporting.

**Features:**
- AWS resource discovery (EC2, S3, Lambda, RDS)
- Docker resource discovery
- Kubernetes resource discovery
- Concurrent discovery
- Report generation
- Cost tracking ready

**Usage:**
```bash
./cloud_inventory.wyn
```

**Replaces:** Manual inventory spreadsheets and multiple CLI commands.

## Common Patterns

### Error Handling
```wyn
const result = risky_operation()
match result {
    ok(value) => {
        # Success path
    }
    err(message) => {
        log.error(message)
        os.exit(1)
    }
}
```

### Concurrent Execution
```wyn
spawn { check_service_1() }
spawn { check_service_2() }
spawn { check_service_3() }
task_join_all()  # Wait for all
```

### Configuration
```wyn
const config = os.getenv_default("CONFIG", "default_value")
```

### Logging
```wyn
log.info("Operation started")
log.warn("Warning condition")
log.error("Critical error")
```

## Why Wyn for DevOps?

### vs Bash
- ✅ Type safety catches errors before runtime
- ✅ Proper error handling (no `set -e` hacks)
- ✅ Readable, maintainable code
- ✅ Cross-platform (no bash on Windows)

### vs Python
- ✅ 10x faster startup (<100ms vs 1s+)
- ✅ Compiled binary (no dependency hell)
- ✅ Better concurrency (spawn blocks vs threading)
- ✅ Simpler syntax for scripts

### vs Go
- ✅ Simpler syntax (Python-like)
- ✅ Better for scripts (no boilerplate)
- ✅ Same performance
- ✅ Easier error handling

## Running These Examples

```bash
# Make executable
chmod +x examples/*.wyn

# Run directly
./examples/aws_deploy.wyn

# Or use wyn run
wyn run examples/docker_healthcheck.wyn

# Compile to binary
wyn compile examples/k8s_deploy.wyn
./a.out
```

## Customization

All examples use environment variables for configuration, making them easy to adapt to your infrastructure:

```bash
export DB_NAME=my_database
export S3_BUCKET=my-backups
export NAMESPACE=production
./db_backup.wyn
```

## Production Ready

These examples demonstrate production-grade patterns:
- ✅ Proper error handling
- ✅ Logging and observability
- ✅ Configuration via environment
- ✅ Validation and verification
- ✅ Rollback capabilities
- ✅ Concurrent execution
- ✅ Exit codes for CI/CD integration

## Next Steps

1. Adapt these examples to your infrastructure
2. Add your specific monitoring/alerting integrations
3. Integrate with your CI/CD pipelines
4. Replace your bash/Python scripts with Wyn

**Wyn makes infrastructure automation simple, safe, and fast.**
