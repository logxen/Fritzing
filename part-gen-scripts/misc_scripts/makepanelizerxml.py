# usage:
#	makepanelizerxml.py -f [dif file from google docs] 
#	creates a panelizer.xml file that can be used for further steps

import getopt, sys, os, os.path, re
from datetime import date
    
def usage():
    print """
usage:
    makepanelizerxml.py -f [dif file from google docs] 
    creates a panelizer.xml file that can be used for further steps
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
        
    if not(fromName.endswith(".dif")):
        print "file", fromName, "is not .dif"
        usage()
        sys.exit(2)

    
    diffile = open(fromName, "r")
    gotData = 0
    while 1:
        line = diffile.readline()
        if line.startswith("DATA"):
            gotData = 1
            break
            
    if not(gotData):
        print "file", fromName, "has no DATA element"
        return
            
    diffile.readline()    
    diffile.readline()    
    diffile.readline()    
    line = diffile.readline()
    if not(line.startswith("BOT")):
        print "file", fromName, "has no initial BOT element"
        return

    
    fields = []
    while 1:
        pair = diffile.readline()
        value = diffile.readline()
        if pair.startswith("1"):
            value = value.replace('"', "")
            value = value.replace("\n", "")
            fields.append(value)
        elif pair.startswith("0"):
            s = pair.split(",")
            if len(s) > 2:
                print "file", fromName, "unexpected format 1"
                return
                
            fields.append(s[1])
        elif value.startswith("BOT"):
            break
        else:
            print "file", fromName, "unexpected format 2"
            return           
            
    #print fields        
    
    countIndex = 0
    orderNumberIndex = 0
    filenameIndex = 0
    optionalIndex = 0
    try:
        countIndex = fields.index("count")
        orderNumberIndex = fields.index("order-nr")
        filenameIndex = fields.index("Filename")
        optionalIndex = fields.index("count optional")
    except:
        print "file", fromName, "missing 'count', 'order-nr' or 'Filename' field"
        return
        
    lines = []
    done = 0
    while not(done):
        values = []
        while 1:
            pair = diffile.readline()
            value = diffile.readline()
            if pair.startswith("1"):
                value = value.replace('"', "")
                value = value.replace("\n", "")
                values.append(value)
            elif pair.startswith("0"):
                s = pair.split(",")
                if len(s) > 2:
                    print "file", fromName, "unexpected format 3"
                    return
                
                values.append(s[1])
            elif value.startswith("BOT"):
                break
            elif value.startswith("EOD"):
                done = 1
                break
            else:
                print "file", fromName, "unexpected format 4"
                return
             

        optional = 0
        try:
            optional = int(values[optionalIndex])
        except ValueError:
            pass

        filename = values[filenameIndex]
        if len(filename) == 0:
            continue
         
        orderNumber = values[orderNumberIndex].replace("\n", "")
        if len(orderNumber) == 0:
            continue
        
        try:
            count = int(values[countIndex])
        except ValueError:
            print "file", fromName, "invalid count"
            return
            
        print filename, orderNumber, count
        xml =  "<board name='{0}_{1}_{2}' requiredCount='{1}' maxOptionalCount='{3}' inscription='{0}' inscriptionHeight='2mm' originalName='{2}' />\n"
        if orderNumber == "products":
            xml =  "<board name='{2}' requiredCount='{1}' maxOptionalCount='{3}' inscription='' inscriptionHeight='2mm' originalName='{2}' />\n"
        xml = xml.format(orderNumber, count, filename, optional)
        lines.append(xml)
    
    
    outputDir = os.path.dirname(fromName)
    outfile = open(os.path.join(outputDir, "panelizer.xml"), "w")
    today = date.today()
    outfile.write("<panelizer width='550mm' height='330mm' spacing='6mm' border='0mm' outputFolder='{0}' prefix='{1}'>\n".format(outputDir, today.strftime("%Y.%m.%d")))
    outfile.write("<paths>\n")
    outfile.write("<path>{0}</path>\n".format(outputDir))
    productDir = outputDir
    productDir = os.path.split(productDir)[0]
    productDir = os.path.split(productDir)[0]
    productDir = os.path.join(productDir, "products")
    outfile.write("<path>{0}</path>\n".format(productDir))
    outfile.write("</paths>\n")
    outfile.write("<boards>\n")    
    for l in lines:
        outfile.write(l)
    outfile.write("</boards>\n")    
    outfile.write("</panelizer>\n")    
    outfile.close()
    

    


if __name__ == "__main__":
    main()



