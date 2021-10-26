# MapServer Security Policy

## Reporting a Vulnerability in MapServer

Security/vulnerability reports should not be submitted through GitHub tickets or the public mailing lists, but instead please send your report 
to the email address: **mapserver-security nospam @ osgeo.org** (remove the blanks and ‘nospam’).  

Please follow the general guidelines for bug 
submissions, when describing the vulnerability (see https://mapserver.org/development/bugs.html).

## Supported Versions

The MapServer PSC (Project Steering Committee) will release patches for security vulnerabilities 
for the last release branch of the **two most recent release series** (such as 8.x, 7.x. 6.x, etc...). 
Patches will only be provided **for a period of three years** from the release date of the current series.
For example, once 8.0 is released, then only 8.0.x and 7.6.x will be supported/patched and 7.6.x will
only be supported for three years from the date of the 8.0 series release.

Currently, the following versions are supported:

| Version | Supported          |
| ------- | ------------------ |
| 7.6.x   | :white_check_mark: |
| 7.4.x   | :x:                |
| 7.2.x   | :x:                |
| 7.0.x   | :x:                |
| 6.4.x   | :x:                |
| < 6.4   | :x:                |

Note: _MapServer 7.0.0 was released on 2015-07-24._

## Version Numbering: Explained

x
: Major release series number.
: Major releases indicate substantial changes to the software and 
  backwards compatibility is not guaranteed across series. Current 
  release series is 7.

y
: Minor release series number.
: Minor releases indicate smaller, functional additions or improvements 
  to the software and should be generally backwards compatible within a 
  major release series. Users should be able to confidently upgrade 
  from one minor release to another within the same release series, so 
  from 7.4.x to 7.6.x.

z
: Point release series number.
: Point releases indicate maintenance releases - usually a combination of 
  bug and security fixes and perhaps small feature additions. Backwards 
  compatibility should be preserved and users should be able to confidently 
  upgrade between point releases within the same release series, 
  so from 7.6.4 to 7.6.5.
