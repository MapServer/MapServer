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
reopen with missing/relevant information if still valid.

Typically, issues fall in this state for one of the following reasons:
- Hard, impossible or not enough information to reproduce
- Missing test case
- Lack of a champion with interest and/or funding to address the issue
"""

AUTOCLOSE_LABEL = "No Recent Activity"


def fetch_issues(headers):
    issues = []
    url = ("https://api.github.com/repos/mapserver/mapserver/issues?"
            "per_page=100")

    while url:
        print "Fetching %s" % url
        r = requests.get(url, headers=headers)
        r.raise_for_status()
        time.sleep(1)
        issues.extend(r.json())

        match = re.match(r'<(.*?)>; rel="next"', r.headers['Link'] or '')
        url = match.group(1) if match else None

    return issues


def close_issue(issue, headers):
    """Attempt to close an issue and return whether it succeeded."""
    r = requests.post("https://api.github.com/repos/mapserver/mapserver/"
            "issues/%s" % issue['number'],
            data=json.dumps({'state': 'closed'}), headers=headers)

    try:
        r.raise_for_status()
        time.sleep(1)
        return True
    except requests.HTTPError:
        return False


def post_issue_comment(issue, comment_text, headers):
    """Attempt to post an issue comment and return whether it succeeded."""
    r = requests.post("https://api.github.com/repos/mapserver/mapserver/"
            "issues/%s/comments" % issue['number'],
            data=json.dumps({'body': comment_text}), headers=headers)

    try:
        r.raise_for_status()
        time.sleep(1)
        return True
    except requests.HTTPError:
        return False


def add_issue_label(issue, label, headers):
    """Attempt to add a label to the issue and return whether it succeeded."""
    r = requests.post("https://api.github.com/repos/mapserver/mapserver/"
            "issues/%s/labels" % issue['number'],
            data=json.dumps([label]), headers=headers)

    try:
        r.raise_for_status()
        time.sleep(1)
        return True
    except requests.HTTPError:
        return False


def close_old_issues(close_before, headers):
    all_issues = fetch_issues(headers)

    for issue in all_issues:
        issue_last_activity = datetime.datetime.strptime(
            issue['updated_at'], "%Y-%m-%dT%H:%M:%SZ")

        print "Processing issue %s with last activity at %s" % (issue['number'], issue_last_activity)

        if issue_last_activity < close_before and issue["state"] == "open":
            if post_issue_comment(issue, AUTOCLOSE_MESSAGE, headers):
                print "    Added comment to old issue %s" % issue['number']
            else:
                print "    Error adding comment to old issue %s" % issue['number']
                continue

            if add_issue_label(issue, AUTOCLOSE_LABEL, headers):
                print "    Added label to old issue %s" % issue['number']
            else:
                print "    Error adding label to old issue %s" % issue['number']

            if close_issue(issue, headers):
                print "    Closed old issue %s" % issue['number']
            else:
                print "    Error closing old issue %s" % issue['number']


if __name__ == '__main__':
    close_before = datetime.datetime.today() - datetime.timedelta(days=366)

    print
    print "This script closes all issues with no activity within the last year."
    print

    github_token = getpass.getpass("Please provide an oauth token: ")
    print

    headers = {"Authorization": "token %s" % github_token}
    close_old_issues(close_before, headers)
