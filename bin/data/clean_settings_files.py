from xml.etree.ElementTree import ElementTree
import os

'''
Clean the settings file from parameters such as
<fullscreen__F_>1</fullscreen__F_>
This allows for configuration loaded to not change some of the display modes
'''
elementsToRemove =['fullscreen__F_','show_gui__G_','Source_mode__z_']
def clean():
    currentDir = os.path.dirname(os.path.realpath(__file__))
    print(currentDir)

    for filename in os.listdir(currentDir+'\\'):
        if not filename.endswith('.xml'):
            continue
        print ('Cleaning {}'.format(filename))
        cleanFile(os.path.join(currentDir,filename))

def cleanFile(filename):
    print (filename)
    tree = ElementTree()
    tree.parse(filename)
    root = tree.getroot()
    for element in elementsToRemove:
        print(element)
        elementNode = tree.find(element)
        print(elementNode)
        if(elementNode is not None):
            root.remove(elementNode)
        tree.write(filename)

if __name__ == '__main__':
    clean()
    input("Done. Pressed any key to close.")
