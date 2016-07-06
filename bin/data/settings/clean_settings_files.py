from xml.etree.ElementTree import ElementTree
import os

'''
Clean the settings file from parameters such as
<fullscreen__F_>1</fullscreen__F_>
This allows for configuration loaded to not change some of the display modes
We can use XPath to go to sub parameter such as 
Transition_time which is a child of settings_transition
'''
elementsToRemove =['fullscreen__F_','show_gui__G_',
'Source_mode__z_',
'Send_to_Spout',
'average_FPS',
'minimum_FPS',
'.//settings_transition/Transition_time',
'.//settings_transition/Settings_file',
'.//settings_transition/Jump_between_interval',
'.//settings_transition/Jump_between_states',
'.//settings_transition/UnknownName']
elementsToRemoveSettingsXml = ['fullscreen__F_','show_gui__G_',
'Source_mode__z_',
'Send_to_Spout',
'.//settings_transition/Settings_file',
'.//settings_transition/UnknownName']

SETTINGS_FILE_NAME = 'settings.xml'

def clean(subFolder=''):
    currentDir = os.path.dirname(os.path.realpath(__file__))
    if(subFolder):
        currentDir = os.path.join(currentDir, subFolder)
    print(currentDir)

    for filename in os.listdir(currentDir):
        if not filename.endswith('.xml') or filename.endswith(SETTINGS_FILE_NAME):
            continue
        print ('Cleaning {}'.format(filename))
        cleanFile(os.path.join(currentDir,filename), elementsToRemove)

    settingsXmlFile = os.path.join(currentDir, SETTINGS_FILE_NAME)
    if(os.path.isfile(settingsXmlFile)):
        cleanFile(settingsXmlFile , elementsToRemoveSettingsXml)

def cleanFile(filename, elementsToBeRemove):
    tree = ElementTree()
    tree.parse(filename)
    parent_map = dict((c,p) for p in tree.getiterator() for c in p)
    
    root = tree.getroot()
    for element in elementsToBeRemove:
        elementNode = tree.find(element)
        if(elementNode is not None):
            parent_map[elementNode].remove(elementNode)
        tree.write(filename)

if __name__ == '__main__':
    clean()
    clean('pseyesettings')
