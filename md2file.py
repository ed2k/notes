"""
merge tree of .md files to one file
"""
import re
NEWLINE = '\n'

def write_and_clear(title, buffer, fd):
    fd.write(' '.join(title) + NEWLINE)
    if buffer:
        fd.write(NEWLINE.join(buffer))
    fd.write(NEWLINE)
    return (None, list())


def get_new_title(history, words):
    temp_title = list()
    for word in words[1:]:
        word = word.lower().replace('[', '').replace(']', '').replace(')', '').replace('(', '')
        temp_title.append(word)
        title = ' '.join(temp_title)
        if title not in history:
            return title


def convert_one_file(prefix, filename, target_fd):
    """prefix list of keywords
    """
    heads = list()
    history = list()
    buffer = list()
    title = None
    for line in open(filename):
        if line.strip() == '':
            if title:
                title, buffer = write_and_clear(prefix + [title], buffer, target_fd)
        else:
            words = re.findall('([^\[\]\s\(\)]+)', line)
            print(words, [line])
            if len(words) == 2 and words[0] == '#':
                if words[1] not in prefix:
                    if len(heads) == 0:
                        heads.append(words[1])
                    elif words[1] not in heads:
                        heads[0] = words[1]
            elif len(words) == 2 and words[0] == '##':
                if words[1] not in prefix:
                    if len(heads) == 1:
                        if words[1] not in heads:
                            heads.append(words[1])
                    elif len(heads) == 2:
                        if words[1] not in heads:
                            heads[1] = words[1]
            elif len(words) > 1 and words[0] == '-':
                if title:
                    title, buffer = write_and_clear(prefix + [title], buffer, target_fd)
                temp_title = list()
                for word in words[1:]:
                    word = word.lower()
                    temp_title.append(word)
                    title = ' '.join(temp_title)
                    if title not in history:
                        history.append(title)
                        break
                buffer.append(line[2:])
            else:
                if not title:
                    temp_title = list()
                    for word in words:
                        word = word.lower()
                        temp_title.append(word)
                        title = ' '.join(temp_title)
                        if title not in history:
                            history.append(title)
                            break
                buffer.append(line)

import os


def main():
    fd = open('test.txt', 'w')
    for d, subd, files in os.walk('./'):
        prefix = d.split('/')[1:]
        if len(prefix) == 1 and prefix[0] == '':
            # remove first level file
            continue
        for fname in files:
            if fname[-3:] != '.md' or fname[0] == '.':
                continue
            print(f' {prefix} {fname}')
            fname_first = fname[:-3]
            prefix_fname = prefix
            if fname_first not in prefix:
                prefix_fname.append(fname_first)
            convert_one_file(prefix_fname, f'{d}/{fname}', fd)

main()