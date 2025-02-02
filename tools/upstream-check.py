#!/usr/bin/env python2

# Check for new versions of third-party libraries

from __future__ import print_function
import datetime
import json
import urllib

TAGGED_REPOS = {
    'pybind/pybind11': 'v2.4.2',
    'taocpp/PEGTL': '2.4.0',
    'cxong/tinydir': '1.2.4',
    'madler/zlib': 'v1.2.11',
}

NOT_TAGGED_REPOS = {
    'chadaustin/sajson': 'include/sajson.h',
    'sgorsten/linalg': 'linalg.h',
    'nothings/stb': 'stb_sprintf.h',
}

def check_tags():
    for repo, version in TAGGED_REPOS.items():
        url = 'https://api.github.com/repos/%s/releases/latest' % repo
        response = urllib.urlopen(url)
        data = json.load(response)
        if 'tag_name' in data:
            latest_tag = data['tag_name']
        else:
            url = 'https://api.github.com/repos/%s/tags' % repo
            response = urllib.urlopen(url)
            data = json.load(response)
            latest_tag = data[0]['name']
        mark = ('   !!!' if version != latest_tag else '')
        print('%-18s %10s %10s%s' % (repo, version, latest_tag, mark))

def check_recent_commits():
    since = datetime.datetime.now() - datetime.timedelta(days=30)
    for repo, filename in NOT_TAGGED_REPOS.items():
        url = 'https://api.github.com/repos/%s/commits?path=%s&since=%s' % (
              repo, filename, since)
        response = urllib.urlopen(url)
        data = json.load(response)
        info = '-'
        if data:
            info = data[0]['commit']['committer']['date'][:10]
        print('%-18s  modified: %s' % (filename.split('/')[-1], info))

try:
    check_tags()
    check_recent_commits()
except IOError as e:
    print(e)
