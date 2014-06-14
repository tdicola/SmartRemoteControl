import os
import xml.etree.ElementTree as et

# Change to directory of script so relative file references work.
os.chdir(os.path.dirname(os.path.abspath(__file__)))

# Name of configuration file.
FILE_NAME = 'activities.xml'


def get_activities():
	# Open the config XML file and parse all the activities.
	config = []
	for child in et.parse(FILE_NAME).iter('activity'):
		# Create a dictionary with the name and code list for each activity.
		config.append({'name': child.find('name').text,
					   'codes': [x.text for x in child.iter('code')]
					   })
	return config
