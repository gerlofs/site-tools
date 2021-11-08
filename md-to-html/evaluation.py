class EvaluationError(Exception):
    def __init__(self, msg): 
        print(msg)
        pass

class Evaluator():
    def __init__(self, line_array):
        self.line_array = line_array
        self.selected_line = 0
        self.paragraph_open = False

    def evaluate_line(self, line):
        return

    def evaluate_character(self, s):
        return

    def next_line(self):
        self.selected_line = self.selected_line + 1;
        if ( self.selected_line >= len(self.line_array) ):
            return None
        else:
            return self.line_array[self.selected_line]

    def correct(self, line, insertstr, index, offset):
        line = "{}{}{}".format(line[0:index],   \
                                insertstr,      \
                                line[index+offset:])
        return line

    def bold(self, s, i):
        """
            Using `find()` to locate a closing bold indicator. 
            If one doesn't exist in the current line, Search the
                next line, and the next, and so on. If we reach the end
                of the line array, we raise an error.
            
            s <String>: Input string (start_string[i:]) - e.g.
                        "**snippet** of text."
            i <Integer>: Index from which the starting character of the 
                            string is taken.
        """
        cached_line = self.selected_line
        whole_line = self.correct(self.line_array[self.selected_line],  \
                                                                "<b>",  \
                                                                i,      \
                                                                2)
        s = self.line_array[self.selected_line] = whole_line
        
        posn = s.find('**')
        while ( posn == -1 ):
            s = self.next_line()
            if ( s is None ): 
                raise EvaluationError("No closing **")
            posn = s.find('**')
        
        # Alter line once closing character is found (TODO). 
        whole_line = self.correct(self.line_array[self.selected_line],  \
                                                                "</b>", \
                                                                posn,   \
                                                                2)
        self.line_array[self.selected_line] = whole_line
        self.selected_line = cached_line
            
        return i+3

    def italic(self, s, i):
        cached_line = self.selected_line
        whole_line = self.correct(self.line_array[self.selected_line],  \
                                                                "<i>",  \
                                                                i,      \
                                                                1)
        s = self.line_array[cached_line] = whole_line
        
        posn = s.find('*')
        # If we find a single asterisk...
        while ( posn == -1 or ((posn >= 0) and (s[posn+1:posn+2] == '*'))):
            s = self.next_line()
            if ( s is None ): 
                raise EvaluationError("No closing *")
            posn = s.find('*')
        
        whole_line = self.correct(self.line_array[self.selected_line],  \
                                                                "</i>", \
                                                                posn,   \
                                                                1)
        self.line_array[self.selected_line] = whole_line
        self.selected_line = cached_line
        return i+3

    def heading(self, s, level):
        tags = ["<h{}>".format(str(level)), "</h{}>".format(str(level))]
        s = tags[0] + s[level:] + tags[1]
        self.line_array[self.selected_line] = s
        return len(s) 

    def hyperlink(self, s, i):
        """
            Change a block of text to a html anchor + href tag.
            Seek the end of the link by finding a closing bracket ')'
                and a central 
        """

        link_text_end_posn = s.find('](')
        # Find closing bracket after link closing bracket.
        # Done this way to allow links to contain brackets.
        hyperlink_end_posn = s[link_text_end_posn:].find(')')
        hyperlink_end_posn = link_text_end_posn + hyperlink_end_posn
        if ( hyperlink_end_posn == -1 ):
            raise EvaluationError("Unable to find closing hyperlink"\
                                    "bracket ')'...")
        
        hyperlink_html = \
            '<a href="{}" target="_blank" rel="noopener noreferrer">{}</a>'\
            .format(s[link_text_end_posn+2:hyperlink_end_posn], \
                    s[1:link_text_end_posn])
        new_line = self.correct(self.line_array[self.selected_line],    \
                                hyperlink_html,                         \
                                i,
                                (hyperlink_end_posn+1))
        self.line_array[self.selected_line] = new_line
        return (hyperlink_end_posn+i)+1

    def codeblock(self, index, inline=False):
        if ( inline ):
            tags = ["<code>", "</code>"]
            offset = 1
            findstr = '`'
        else:
            tags = ["<pre><code>", "</code></pre>"]
            offset = 3
            findstr = '```'
        
        # Retrieve the line we're on.
        line = self.line_array[self.selected_line]
        # Alter the line at the start index, then attempt to find the 
        #   closing character and replace the line.
        line = self.correct(line, tags[0], index-offset, offset)
        posn = line.find(findstr)
        self.line_array[self.selected_line] = line
        
        # If we cannot find the line, we search through the remaining lines
        #   in the array.
        while ( posn == -1 ):    
            line = self.next_line()
            if ( line is None ):
                raise EvaluationError("No closing code block [{}]..."\
                                        .format(findstr))
            posn = line.find(findstr)
        
        # Finally, replace the line again with the final correction.
        line    = self.line_array[self.selected_line] \
                = self.correct(line, tags[1], posn, offset)

        # As we don't want to parse anything in the code block we 
        #   return the line and index within that line AFTER the block
        #   is closed.
        print(line[posn+offset:])
        return (line, posn+offset)
   
    def paragraph(self, line):
        # If starting character is not special and we're not already in 
        #   a paragraph block, open one.
        self.paragraph_open = True
        line = "<p>" + line
        self.line_array[self.selected_line] = line
        return (line, 3)

    def newline(self, line, index):
        # If index is zero, end the paragraph.
        # Otherwise, insert a <br>.
        cached_line_number = self.selected_line
        line_to_correct = self.line_array[self.selected_line]
        
        if ( index > 0 ):
            line_to_correct = line_to_correct[0:index] \
                                + "<br>" \
                                + line_to_correct[index+1:]
            line = line_to_correct
            index = index + 4
        else:
            line_to_correct = "</p>"
            self.paragraph_open = False
            line = self.next_line()
        
        self.line_array[cached_line_number] = line_to_correct
        return (line, index)

    def parse(self, line, index):
        """
            Parse through a line, calling the correct method to 
                handle special markdown characters - e.g. `bold`
                for '**'. Once we finish a line, move to the next
                'til we have no lines left.
        """
        while ( (line is not None) and (index is not None) ): 
            cur = line[index:index+1]
            nxt = line[index+1:index+2]
            # Grab next line """
            if ( cur == '' ):
                return self.parse(self.next_line(), 0)
            # Bold and italic """
            elif ( cur == '*' ):
                if ( nxt == '*' ):
                    index = self.bold(line, index)
                    line = self.line_array[self.selected_line]
                else:
                    index = self.italic(line, index)
                    line = self.line_array[self.selected_line]
            # Headings: """
            elif ( index == 0 and cur == '#' ):
                # Count number of sequential pound occurances.
                occur = 1
                while ( nxt == '#' ):
                    index = index + 1
                    nxt = line[index+1:index+2]
                    occur = occur + 1
                index = self.heading(line, occur)
            # Hyperlink """
            elif ( cur == '[' and (line[index:].find('](') != -1) ):
                index = self.hyperlink(line[index:], index) 
                line = self.line_array[self.selected_line]
            # Code-block
            elif ( cur == '`' ):
                if ( nxt == '`' and line[index+2:index+3] == '`' ):
                    line, index = self.codeblock(index+3, inline=False)
                else:
                    line, index = self.codeblock(index+1, inline=True)
            # Horizontal rule
            elif ( index == 0 and cur == '-' and nxt == '-' \
                    and (line[index+2:index+3] == '-') ):
                self.line_array[self.selected_line] = '<hr>'
                line = self.next_line()
                index = 0 
            # Newlines:
            elif ( cur == '\n' ):
                line, index = self.newline(line[index:], index)
            elif ( (index == 0) and (not self.paragraph_open) ):
                line, index = self.paragraph(line)
            else:
                index = index + 1
        
    def evaluate(self):
        self.parse(self.line_array[self.selected_line], 0)

"""e = Evaluator(lines)
e.evaluate()
print(e.line_array)"""
