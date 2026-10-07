#!/usr/bin/env python

import sys
import urllib
import optparse

CONDOR_FORTUNE_WIKI_URL = 'https://condor-wiki.cs.wisc.edu/index.cgi/wiki?p=CondorUserFortune'

def main(argv=None):
    '''Main entry point'''
    
    if argv is None:
        argv = sys.argv
    
    parser = optparse.OptionParser()
    parser.add_option('-o', '--output', dest='output_filename',
                      help='Write output to FILE', metavar='FILE',
                      action='store', type='string')
    (options, args) = parser.parse_args(argv[1:])

    input_obj = urllib.urlopen(CONDOR_FORTUNE_WIKI_URL)
    input_lines = input_obj.readlines()
    input_obj.close()

    fortune_start = -1
    fortune_end = -1

    for line_number, line in enumerate(input_lines):
        if line.startswith('Condor Tip #1:'):
            fortune_start = line_number
            break
    if fortune_start != -1:
        for line_number, line in enumerate(input_lines[fortune_start:]):
            if line.startswith('</pre></div>'):
                fortune_end = line_number + fortune_start
                break
    
    if fortune_start != -1 and fortune_end != -1:
        fortune_lines = input_lines[fortune_start:fortune_end]
        output_obj = sys.stdout
        if options.output_filename:
            output_obj = open(options.output_filename, 'w')

        for line in fortune_lines:
            line = line.replace('&lt;', '<')
            line = line.replace('&gt;', '>')
            line = line.replace('&quot;', '"')
            output_obj.write(line)
    else:
        print >> sys.stderr, "I wasn't able to determine where " \
                             "the fortune lines are in the wiki page."

    return 0


if __name__ == '__main__':
    sys.exit(main())
