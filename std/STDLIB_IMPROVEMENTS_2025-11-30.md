# Standard Library Improvements - 2025-11-30

## Summary

Enhanced the Wyn standard library with additional utility functions, comprehensive documentation comments, and proper annotations for future runtime support.

## Changes Made

### net.wyn
**New Socket Functions:**
- `set_timeout()` - Set socket timeout in milliseconds
- `set_nonblocking()` - Set socket to non-blocking mode
- `set_reuseaddr()` - Enable SO_REUSEADDR option

**New URL Functions:**
- `build_url()` - Construct URL from components
- `extract_query()` - Extract query string from path
- `strip_query()` - Remove query string from path
- `url_path()` - Extract path from URL
- `url_scheme()` - Extract scheme from URL
- `is_https()` - Check if URL uses HTTPS
- `is_http()` - Check if URL uses HTTP/HTTPS
- `default_port_for_scheme()` - Get default port for common schemes

**New IP Functions:**
- `is_multicast_ip()` - Check for multicast addresses (224.0.0.0/4)
- `is_link_local_ip()` - Check for link-local addresses (169.254.0.0/16)
- `is_broadcast_ip()` - Check for broadcast address (255.255.255.255)
- `is_unspecified_ip()` - Check for unspecified address (0.0.0.0)
- `network_address()` - Calculate network address with CIDR prefix
- `broadcast_address()` - Calculate broadcast address with CIDR prefix

**Documentation:**
- Added "Requires: Runtime support for socket syscalls" to all socket functions
- Added section headers for better organization

### http.wyn
**New Client Methods:**
- `patch()` - HTTP PATCH request
- `head()` - HTTP HEAD request
- `options()` - HTTP OPTIONS request

**New Method Constants:**
- `method_trace()` - TRACE method
- `method_connect()` - CONNECT method

**New Status Codes:**
- `status_not_implemented()` - 501
- `status_bad_gateway()` - 502
- `status_service_unavailable()` - 503
- `status_gateway_timeout()` - 504
- `status_moved_permanently()` - 301
- `status_found()` - 302
- `status_not_modified()` - 304
- `status_temporary_redirect()` - 307
- `status_permanent_redirect()` - 308

**New Status Checks:**
- `is_error()` - Check for any error (4xx or 5xx)
- `is_informational()` - Check for informational (1xx)

**New Header Functions:**
- `header_key()` - Extract key from header string
- `header_value()` - Extract value from header string
- Common header name constants (content_type, authorization, etc.)
- Common content type constants (json, html, text, xml, form, multipart)

**New Response Builders:**
- `text_response()` - Plain text response
- `xml_response()` - XML response
- `redirect_response()` - 302 redirect
- `permanent_redirect_response()` - 301 redirect
- `error_response()` - Error response with message

**New Method Checks:**
- `has_request_body()` - Check if method typically has request body
- `has_response_body()` - Check if method typically has response body
- `is_valid_method()` - Validate HTTP method string

**Documentation:**
- Added "Requires: Runtime support for HTTP client/server" to all HTTP functions
- Added section headers for better organization
- Added "Stage 1: Will support array building" to `parse_header()`

### fs.wyn
**New File Operations:**
- `read_lines()` - Read file as array of lines
- `write_lines()` - Write array of lines to file
- `mkdir_all()` - Create directory with all parents
- `remove_all()` - Remove file or directory recursively
- `copy()` - Copy file from src to dst
- `move()` - Move/rename file
- `file_size()` - Get file size in bytes

**New Path Functions:**
- `has_parent_refs()` - Check for parent directory references (..)
- `is_hidden()` - Check if path is hidden (starts with .)
- `is_text_file()` - Check if path is text file by extension
- `sanitize_filename()` - Remove invalid characters from filename
- `safe_join()` - Join and normalize paths safely

**New Extension Constants:**
- `ext_text()`, `ext_json()`, `ext_xml()`, `ext_html()`, `ext_css()`, `ext_js()`, `ext_md()`, `ext_wyn()`

**Documentation:**
- Added "Requires: Runtime support for filesystem syscalls" to all I/O functions
- Added "Stage 1: Will support array building" to `read_lines()` and `list()`
- Added section headers for better organization

## Testing

All 31 tests pass successfully:
- 10 core tests (arithmetic, array, bitwise, builtins, comparison, control_flow, function, optional, string, struct)
- 21 example tests (array_iter, assert, bitwise, break_continue, char, compound, conversion, enum, file, float, full, generic, match, math, method_syntax, nested_struct, optional, range, result, slice, string)

## Notes

- All new functions maintain consistency with existing Wyn syntax
- Functions requiring runtime support are clearly marked
- Functions that will need array building in Stage 1 are annotated
- No breaking changes to existing API
- All utility functions are pure and side-effect free where possible
