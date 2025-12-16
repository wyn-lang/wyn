# AWS SDK for Wyn

Production-ready AWS operations for Wyn language.

## Installation

```bash
wyn pkg install github.com/wyn-lang/aws
```

## Prerequisites

- AWS CLI installed and configured
- Valid AWS credentials

## Usage

```wyn
import aws

fn main() {
    # Set region and profile
    aws.set_region("us-east-1")
    aws.set_profile("default")
    
    # S3 operations
    const buckets = aws.s3_list_buckets()
    aws.s3_upload("my-bucket", "file.txt", "/local/file.txt")
    
    # EC2 operations
    const instances = aws.ec2_list_instances()
    aws.ec2_start_instance("i-1234567890abcdef0")
    
    # Lambda operations
    const result = aws.lambda_invoke("my-function", "{\"key\":\"value\"}")
    
    # DynamoDB operations
    aws.dynamodb_put_item("my-table", "{\"id\":{\"S\":\"123\"}}")
}
```

## Available Functions

### S3
- `s3_list_buckets() -> [str]`
- `s3_upload(bucket: str, key: str, file: str) -> bool`
- `s3_download(bucket: str, key: str, local_file: str) -> bool`
- `s3_delete(bucket: str, key: str) -> bool`

### EC2
- `ec2_list_instances() -> [str]`
- `ec2_start_instance(instance_id: str) -> bool`
- `ec2_stop_instance(instance_id: str) -> bool`

### Lambda
- `lambda_invoke(function: str, payload: str) -> str`
- `lambda_list_functions() -> [str]`

### DynamoDB
- `dynamodb_put_item(table: str, item: str) -> bool`
- `dynamodb_get_item(table: str, key: str) -> str`

### CloudFormation
- `cf_deploy_stack(stack_name: str, template_file: str) -> bool`
- `cf_delete_stack(stack_name: str) -> bool`

### Utilities
- `set_region(region: str)`
- `set_profile(profile: str)`
