# Infrastructure/DevOps Examples

This directory contains practical infrastructure and DevOps examples that demonstrate Wyn's capabilities for real-world automation tasks.

## Examples

### 1. log_analyzer.wyn
**Log File Analysis and Monitoring**
- Processes application log files to extract error counts and response times
- Generates alerts based on error rate thresholds
- Uses: `io`, `regex`, `time`, `log`, `collections`

### 2. api_monitor.wyn
**API Health Monitoring**
- Monitors multiple API endpoints for availability and response times
- Tracks service health across different endpoints
- Generates structured health reports
- Uses: `net`, `time`, `log`, `json`, `collections`

### 3. backup_automation.wyn
**Automated Backup System**
- Creates compressed backups with timestamps
- Implements backup rotation (keeps N most recent backups)
- Generates checksums for integrity verification
- Uses: `io`, `os`, `time`, `compress`, `log`, `hash`

### 4. system_monitor.wyn
**System Resource Monitoring**
- Tracks CPU, memory, and disk usage
- Generates alerts when thresholds are exceeded
- Logs metrics for historical tracking
- Uses: `os`, `io`, `time`, `log`, `regex`, `collections`

### 5. config_manager.wyn
**Configuration Management**
- Validates configuration files before deployment
- Manages configs across different environments
- Encrypts sensitive data for production
- Verifies deployment integrity with checksums
- Uses: `io`, `json`, `hash`, `log`, `collections`, `encoding`

### 6. network_health.wyn
**Network Health Checker**
- Tests connectivity to multiple hosts
- Checks port availability
- Monitors HTTP endpoint health
- Generates comprehensive network reports
- Uses: `net`, `time`, `log`, `collections`, `regex`

### 7. data_pipeline.wyn
**Data Processing Pipeline**
- Validates and transforms incoming data records
- Routes data to different queues based on content
- Provides detailed processing statistics
- Handles error records gracefully
- Uses: `io`, `json`, `time`, `log`, `regex`, `hash`, `collections`, `encoding`

### 8. deployment_automation.wyn
**Application Deployment System**
- Creates deployment packages with versioning
- Performs automated deployments with health checks
- Implements automatic rollback on failure
- Maintains deployment history and reports
- Uses: `io`, `os`, `time`, `log`, `hash`, `collections`, `compress`

## Running the Examples

```bash
# Compile and run any example
./build/wyn run examples/log_analyzer.wyn
./build/wyn run examples/api_monitor.wyn
./build/wyn run examples/backup_automation.wyn
# ... etc

# Or compile first, then run
./build/wyn compile examples/system_monitor.wyn
./a.out
```

## Key Features Demonstrated

- **File I/O Operations**: Reading, writing, and managing files and directories
- **Network Operations**: HTTP requests, DNS resolution, connectivity testing
- **Data Processing**: JSON parsing, regex matching, data transformation
- **System Integration**: Environment variables, process management, system metrics
- **Security**: Hashing, encoding, checksum verification
- **Logging**: Structured logging with different severity levels
- **Compression**: Creating and extracting tar.gz archives
- **Error Handling**: Validation, threshold checking, graceful failure handling

## Use Cases

These examples cover common infrastructure and DevOps scenarios:

- **Monitoring**: System metrics, application health, network connectivity
- **Automation**: Backups, deployments, configuration management
- **Data Processing**: Log analysis, data pipelines, validation
- **Operations**: Health checks, alerting, reporting

Each example is designed to be practical and production-ready, showcasing how Wyn can be used for real infrastructure automation tasks.
