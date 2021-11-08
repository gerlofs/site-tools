"""
CONVERT.PY
    Summary: A tool to convert markdown files to html files without styling.
             the output html files are bare-bones and should be used 
             as inputs to other parts of the site toolchain, rather than
             being published directly - e.g. embedding html files output 
             from convert.py into the content section of a template file
             with stylings to construct final publishable pages.
"""

import sys
import os
from evaluation import Evaluator

""" TODO: 
DONE    1. Add argument parsing with help output (finish).
DONE    2. Add parsing for underlines (hr tag), these are used a lot in blog
            posts as section splitters.
DONE    3. Add parsing for single line code blocks (e.g. `x = 1`)
DONE    4. Fix evaluator parsing - it currently does not work due to Python 
        recursion limit.
        5. Implement logging, add -v to ArgumentParser.
        6. Implement output phase to store html files.
"""

class ArgumentParser():
    def __init__(self):
        self.input_filenames = []
        self.output_filepath = ''
        
        opt_args = self.filter_options(sys.argv[1:])
        opt_args = self.strip_dash(opt_args)
       
        self.contains_help(opt_args)
        
        if ( opt_args == [] ):
            if ( sys.argv[1:] == [] ):
                print("No arguments provided, call for --help...")
                sys.exit(0)
            opt_args = [('a', sys.argv[1:])]
        self.evaluate_options(opt_args)

    def filter_options(self, args):
        optindices = []
        for n, arg in enumerate(args):
            if ( ((len(arg) == 2) and (arg[0] == '-')) \
                or (arg[:2] == '--') ):
                optindices.append(n)

        options = []
        """
            Parse through each indices, for each index read arguments from 
            a[i+1] to a[i+n] where n is the next index. Store option string
            or character and arguments in a tuple and return this.
        """
        for n, i in enumerate(optindices):
            opttuple = ()
            try:
                opttuple = (args[i], [a for a in \
                                args[(i+1):optindices[n+1]]])
            except IndexError:
                opttuple = (args[i], args[(i+1):])
            options.append(opttuple)

        return options

    def strip_dash(self, opt_args):
        for n, t in enumerate(opt_args):
            opt_args[n] = (t[0].replace('-',''), t[1])
        return opt_args
    
    def contains_help(self, opt_args):
        for opt, _ in opt_args:
            if ( opt.lower() in ['h', 'help'] ):
                print("Convert.py\nUsage: python convert.py <markdown file"\
                "path> <options> <option_arguments>\nOptions:\n\t-h, "\
                "--help: Display this helpfile.\n\t-a, --args: Provide a"\
                " single or multiple (as a comma-seperated list) markdown"\
                " files to be processed and output.\n\t-o, --output: "\
                "a destination filepath, by default this is the current"\
                " working directory.\n\t-v, --verbose: Log debug messages")
                sys.exit(0)
    
    def evaluate_options(self, options):
        """
        Parse through option structure and set the corresponding runtime
            variables: e.g. for `-a`, set the list of files to be processed
            to be equal to the arguments given in the proceeding comma-
            seperated-list.
        """
        for option, args in options:
            if ( option in ['o', 'output']):
                self.output_filepath = args[0]
                # TODO: Add check for valid filepath
            elif ( option in ['a', 'args']):
                for a in args:
                    self.input_filenames.append(a.replace(',',''))
            
def open_file(filename, mode, markdown=False):
    if ( markdown and filename.split('.')[-1].lower()\
                        not in ['md', 'markdown'] ):
        raise Exception("Invalid file, input files need to be valid mark"\
                        "down files (as .markdown or .md).")
    fh = open(filename, mode)
    return fh

def close_file(fh):
    fh.close()

def main():
    argp = ArgumentParser()
    evaluator = Evaluator(None)

    for filename in argp.input_filenames:
        
        print("Opening file: {}".format(filename))
        rfh = open_file(filename, "r", markdown=True)
        write_filename = filename.split('.m')[0] + '.html'
        wfh = open_file(write_filename, "w")
         
        # Read into array.
        line_array = rfh.readlines()
        evaluator.line_array = line_array
        evaluator.selected_line = 0
        evaluator.evaluate()
        print(evaluator.line_array)
        wfh.writelines(evaluator.line_array)

        rfh.close()
        wfh.close()

if __name__ == '__main__':
    main()
