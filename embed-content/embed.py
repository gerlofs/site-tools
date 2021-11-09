import sys

class FileHandler():
    def __init__(self, sourcepath, targetpath):
        self.paths = {'src': sourcepath, 'dst': targetpath}
        self.handles = self.open_files(self.paths);
        print("Files opened: ", self.handles)

    def open_files(self, paths):
        handles = {}
        handles['src'] = open(self.paths['src'], "r")
        handles['dst'] = open(self.paths['dst'], "w")
        return handles

def parse_clargs(args):
    clargs = args[1:]
    if ( len(clargs) < 2 ):
        print("Usage: $> python embed.py <source filepath> <target filepath>")
        exit(1);
    else:
        fh = FileHandler(args[1], args[2])

def main():
    parse_clargs(sys.argv)

if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        exit()
