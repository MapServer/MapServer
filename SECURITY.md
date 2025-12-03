# MapServer Security Policy

## Reporting a Vulnerability in MapServer

Security/vulnerability reports should not be submitted through GitHub tickets or the public mailing lists, 
but instead please provide details to the Project Steering Committee via the GitHub [Report a vulnerability](https://github.com/MapServer/MapServer/security/advisories/new) 
option link at the top of this page, or send your report to the email address: 
**mapserver-security nospam @ osgeo.org** (remove the blanks and ‘nospam’).  

Please follow the general guidelines for bug 
submissions, when describing the vulnerability (see https://mapserver.org/development/bugs.html).

## Supported Versions

The MapServer PSC (Project Steering Committee) will release patches for security vulnerabilities 
for the last release branch of the **two most recent release series** (such as 8.x, 7.x, 6.x, etc..., 
where "x" is the most recent release in the series, such as: 7.6.6 being supported, but 
not 7.6.5).  For example, once 8.8 is released, support for 8.6 will be dropped.
 
Patches will only be provided **for a period of three years** from the release date of the current series.
For example, as 8.6 has been released, now 8.6.x will be supported/patched (7.6.x support ended on 2025-09-12, which was three years from the date of the 8.0 series release).

Currently, the following versions are supported:

| Version | Supported          | Support Until |
| ------- | ------------------ |-------------- |
| 8.6.x   | :white_check_mark: |               |
| 8.4.x   | :x:                |               |
| 8.2.x   | :x:                |               |
| 8.0.x   | :x:                |               |
| 7.6.x   | :x:                | 2025-09-12    |
| 7.4.x   | :x:                |               |
| 7.2.x   | :x:                |               |
| 7.0.x   | :x:                |               |
| 6.4.x   | :x:                |               |
| < 6.4   | :x:                |               |

- _MapServer 8.6.0 was released on 2025-12-03_
- _MapServer 8.4.0 was released on 2025-01-15_
- _MapServer 8.2.0 was released on 2024-07-08_
- _MapServer 8.0.0 was released on 2022-09-12_
- _MapServer 7.0.0 was released on 2015-07-24_

## Version Numbering: Explained

version x.y.z means:

**x**
- Major release series number.
- Major releases indicate substantial changes to the software and 
  backwards compatibility is not guaranteed across series. Current 
  release series is 8.

**y**
- Minor release series number.
- Minor releases indicate smaller, functional additions or improvements 
  to the software and should be generally backwards compatible within a 
  major release series. Users should be able to confidently upgrade 
  from one minor release to another within the same release series, so 
  from 7.4.x to 7.6.x.

**z**
- Point release series number.
- Point releases indicate maintenance releases - usually a combination of 
  bug and security fixes and perhaps small feature additions. Backwards 
  compatibility should be preserved and users should be able to confidently 
  upgrade between point releases within the same release series, 
  so from 7.6.4 to 7.6.5.
