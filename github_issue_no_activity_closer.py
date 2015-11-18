#!/usr/bin/env python

import datetime
import getpass
import json
import re
import time

import requests

AUTOCLOSE_MESSAGE = """\
### This is an automated comment

This issue has been closed due to lack of activity. This doesn't mean the \
issue is invalid, it simply got no attention within the last year. Please \
reopen if still valid.
"""

AUTOCLOSE_LABEL = "No Recent Activity"


def fetch_issues(github_auth):
    issues = []
    url = ("https://api.github.com/repos/mapserver/mapserver-import/issues?"
            "per_page=100")
    
    while url:
        print "Fetching %s" % url
        r = requests.get(url, auth=github_auth)
        r.raise_for_status()
        time.sleep(1)
        issues.extend(r.json())
        
        match = re.match(r'<(.*?)>; rel="next"', r.headers['Link'] or '')
        url = match.group(1) if match else None
    
    return issues


def close_issue(issue, github_auth):
    """Attempt to close an issue and return whether it succeeded."""
    r = requests.post("https://api.github.com/repos/mapserver/mapserver-import/"
            "issues/%s" % issue['number'],
            data=json.dumps({'state': 'closed'}), auth=github_auth)
    
    try:
        r.raise_for_status()
        time.sleep(1)
        return True
    except requests.HTTPError:
        return False


def post_issue_comment(issue, comment_text, github_auth):
    """Attempt to post an issue comment and return whether it succeeded."""
    r = requests.post("https://api.github.com/repos/mapserver/mapserver-import/"
            "issues/%s/comments" % issue['number'],
            data=json.dumps({'body': comment_text}), auth=github_auth)
    
    try:
        r.raise_for_status()
        time.sleep(1)
        return True
    except requests.HTTPError:
        return False


def add_issue_label(issue, label, github_auth):
    """Attempt to add a label to the issue and return whether it succeeded."""
    r = requests.post("https://api.github.com/repos/mapserver/mapserver-import/"
            "issues/%s/labels" % issue['number'],
            data=json.dumps([label]), auth=github_auth)
    
    try:
        r.raise_for_status()
        time.sleep(1)
        return True
    except requests.HTTPError:
        return False


def close_old_issues(close_before, github_auth):
    all_issues = fetch_issues(github_auth)
    
    for issue in all_issues:
        issue_last_activity = datetime.datetime.strptime(
            issue['updated_at'], "%Y-%m-%dT%H:%M:%SZ")
        
        print "Processing issue %s with last activity at %s" % (issue['number'], issue_last_activity)
        
        if issue_last_activity < close_before:
            if post_issue_comment(issue, AUTOCLOSE_MESSAGE, github_auth):
                print "    Added comment to old issue %s" % issue['number']
            else:
                print "    Error adding comment to old issue %s" % issue['number']
                continue
            
            if add_issue_label(issue, AUTOCLOSE_LABEL, github_auth):
                print "    Added label to old issue %s" % issue['number']
            else:
                print "    Error adding label to old issue %s" % issue['number']
            
            if close_issue(issue, github_auth):
                print "    Closed old issue %s" % issue['number']
            else:
                print "    Error closing old issue %s" % issue['number']


if __name__ == '__main__':
    close_before = datetime.datetime.today() - datetime.timedelta(days=366)
    
    print
    print "This script will close all issues with no activity for a year."
    print
    
    default_user = "mapserver-bot"
    github_user = (raw_input("GitHub username [%s]: " % default_user)
            or default_user)
    github_pass = getpass.getpass()
    print
    github_auth = (github_user, github_pass)
    
    close_old_issues(close_before, github_auth)
