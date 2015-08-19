# mod_method

Currently only supports replacing a method in a request with a
new value.

## What is it?

The Method directive can be used to replace the request method.
Valid in both per-server and per-dir configurations.

## Syntax

    Method action method1 method2

Where action is one of:

    replace    - Replace the method with the new value.

## Examples

 To set all OPTIONS requests to a GET request, use
 
      Method replace OPTIONS GET
