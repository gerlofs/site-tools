import os
import sys
import re
import shutil


READFILE = "../../site/posts/nostyle.html"
WRITEFILE = "../../site/posts/template.html"

def copy_files():
	for file_path in [READFILE, WRITEFILE]:
		shutil.copy(file_path,\
					file_path[:-5]+"_copy"+file_path[-5:])
	return

def remove_files():
	for file_path in [READFILE, WRITEFILE]:
		os.remove(file_path[:-5]+"_copy"+file_path[-5:])	
	return

	os.remove(READFILE)

def main():
	copy_files()
	post_title = sys.argv[1]	

	expr = 'post-content'
	re_inst = re.compile(expr)
	print(WRITEFILE)
	fp = open(WRITEFILE, "r+")
	lp = ""
	
	file_copy = fp.readlines()
	fp.seek(0,0)
	line_index = 0;

	while ( not re_inst.search(lp)):
		lp  = fp.readline()
		line_index+=1
		if ( not lp ):
			print("Unable to find post-content container div!")
			exit()
	
	lp = ""
	expr = "^<body>"
	re_inst = re.compile(expr)
	rp = open(READFILE, "r")
	
	while ( not re_inst.search(lp)):
		lp  = rp.readline()
		if ( not lp ):
			print("Unable to find body block in nostyle.html")
			exit()
	
	# Move the file pointer to exclude the body tag.
	current_pos = rp.tell()
	rp.seek((current_pos-(len(lp)))+len("<body>"), 0)

	expr = "</body>"
	re_inst = re.compile(expr)

	while True:
		lp = rp. readline()
		if ( not lp ) or ( re_inst.search(lp) ):
 			break
		
		file_copy.insert(line_index, lp)
		line_index+=1

	fp.seek(0, 0)
	fp.writelines(file_copy)	

	rp.close()
	fp.close()
	
	shutil.copy(WRITEFILE, "../../site/posts/"+ post_title + ".html")
	shutil.copy(WRITEFILE[:-5] + "_copy.html", WRITEFILE)
	
	remove_files()	

if __name__ == "__main__":
	main()
