rushbase-tool
=============

Tool for managing community hosting at rushbase.net (upload, rm, mkdir)

Simply use precompiled builds or compile it yourself.

### Current hosting

* Unreal Tournament Community Hosting https://ut.rushbase.net
* Unreal Engine Community Hosting https://ue.rushbase.net

### Usage

On Windows
`rushbase.exe --help`

On Linux
`./rushbase --help`

```
Usage: ./rushbase <options> [<file1> ... <fileN>]
Options:  --help         -h  Print this help
          --destination  -d  Destination (e.g. ut.rushbase.net/<user>/dir)
                            (may be set via environment RUSHBASE_DESTINATION)
          --username     -u  Set username
                            (may be set via environment RUSHBASE_USERNAME)
          --password     -p  Set password
                            (may be set via environment RUSHBASE_PASSWORD)
Operations:
          --mkdir <name> -m <name>  Create directory
          --rm    <name> -r <name>  Remove file
```

### Examples

* Upload multiple files
`rushbase -u maxxy -p mypassword -d ut.rushbase.net/maxxy/screenshots screenshot1.png screenshot2.png`
* Delete above screenshots
`rushbase -u maxxy -p mypassword -d ut.rushbase.net/maxxy/screenshots -r screenshot1.png -r screenshot2.png`
* Create directory
`rushbase -u maxxy -p mypassword -d ut.rushbase.net/maxxy/screenshots -m staging`
* Upload to new directory
`rushbase -u maxxy -p mypassword -d ut.rushbase.net/maxxy/screenshots/staging screenshot1.png screenshot2.png`

### Author 
Damian "Rush" Kaczmarek <rush@rushbase.net>
from Code Charm Ltd

### License 
The MIT License (MIT)

Copyright (c) 2014 Code Charm Ltd
