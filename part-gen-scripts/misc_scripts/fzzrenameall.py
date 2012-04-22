# usage:
#	fzzrenameall.py -f [panelizer xml path]
#	renames fzz file as well as internal fz file.  

# lots of borrowing from http://code.activestate.com/recipes/252508-file-unzip/

import getopt, sys, os, os.path, re, zipfile, shutil
import xml.dom.minidom
    
def usage():
    print """
usage:
    fzzrenameall.py -f [panelizer xml path]
    renames fzz file as well as internal fz file.  
    """
    
           
def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hf:", ["help", "from"])
    except getopt.GetoptError, err:
        # print help information and exit:
        print str(err) # will print something like "option -a not recognized"
        usage()
        sys.exit(2)
    fromName = None
    toName = None
    
    for o, a in opts:
        #print o
        #print a
        if o in ("-f", "--from"):
            fromName = a
        elif o in ("-h", "--help"):
            usage()
            sys.exit(2)
        else:
            assert False, "unhandled option"
    
    if(not(fromName)):
        usage()
        sys.exit(2)
            
    try:
        dom = xml.dom.minidom.parse(fromName)
    except xml.parsers.expat.ExpatError, err:
        print "xml error ", str(err), "in", fromName
        return 
        
    root = dom.documentElement
    outputDir = root.getAttribute("outputFolder")
    if len(outputDir) == 0:
        print "no output folder specified"
        return
        
    boardsNodes = root.getElementsByTagName("boards")
    boards = None
    if (boardsNodes.length == 1):
        boards = boardsNodes.item(0)                
    else:
        print "more or less than one 'boards' element in", fromName
        return
        
    boardNodes = boards.getElementsByTagName("board")
    for board in boardNodes:
        originalName = board.getAttribute("originalName")
        if originalName == None:
            continue
        
        if len(originalName) == 0:
            continue
            
        fromBaseName = originalName
        toBaseName = board.getAttribute("name")
        if len(toBaseName) == 0:
            print "missing name element in board", fromName, board
            return
            
        if fromBaseName == toBaseName:
            # no need to rename
            continue
            
        tempDir = outputDir + "/" + "___temp___"
        shutil.rmtree(tempDir, 1)
        os.mkdir(tempDir)
    
        zf = None
        try:
            zf = zipfile.ZipFile(os.path.join(outputDir, fromBaseName))
            zf.extractall(tempDir)
        except:
            print "unable to unzip", os.path.join(outputDir, fromBaseName)
            continue
    
        #fromFzName = os.path.splitext(fromBaseName)[0] + ".fz"
    
        for i, name in enumerate(zf.namelist()):
            if name.endswith(".fz"):
                fromFzName = name
                break
        
        
        toFzName = os.path.splitext(toBaseName)[0] + ".fz"
    
        print "renaming", fromFzName, toFzName
    
        os.rename(os.path.join(tempDir, fromFzName), os.path.join(tempDir, toFzName))
    
        # helpful examples in http://www.doughellmann.com/PyMOTW/zipfile/
        try:
            import zlib
            compression = zipfile.ZIP_DEFLATED
        except:
            compression = zipfile.ZIP_STORED

        zf = zipfile.ZipFile(outputDir + "/" + toBaseName, mode='w')
        for fn in os.listdir(tempDir):
            zf.write(os.path.join(tempDir, fn), fn, compression)
        zf.close()

        shutil.rmtree(tempDir, 1)


if __name__ == "__main__":
    main()



