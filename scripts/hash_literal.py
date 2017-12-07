import sys


def print_hash_literal(s):
    #assert len(s) == 64
    chars = []
    for x in range(len(s) // 2):
        chars.append('0x' + s[x * 2:(x + 1) * 2])
    print('hash_t FIXME{%s};' % ', '.join(chars))


def main():
    for arg in sys.argv[1:]:
        print_hash_literal(arg)


if __name__ == '__main__':
    main()
