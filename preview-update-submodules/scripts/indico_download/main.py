"""
This script generates the summary report to be placed in the HTCondor past workshop summary page. To create
a new page update the event number below found in the Indico page url and run the script. The script will also download
all associated attachments.

The script will generate a yaml file that can be copied and pasted into the `_event_summaries` folder. Rename the file
in accordance to the other files there, add a title, and convert the yaml to markdown frontmatter.
"""

import os

import yaml
import requests

EVENT_NUMBER = 1201

def main():

    # Move to output directory
    try:
        os.mkdir("./output")
    except:
        pass
    os.chdir("./output")

    # Download the event summary
    d = requests.get(f"https://agenda.hep.wisc.edu/export/event/{EVENT_NUMBER}.json?detail=contributions&pretty=yes").json()
    with open(f"agenda_{EVENT_NUMBER}.yaml", "w") as f:
        yaml.dump(d, f)

    # Download all file attachments

    # Make dir if it doesn't exist
    attachment_dir = f"./{EVENT_NUMBER}-attachments"
    try:
        os.mkdir(attachment_dir)
    except:
        pass


    for contrib in d["results"][0]['contributions']:
        for folder in contrib['folders']:
            for attachment in folder['attachments']:
                if attachment['type'] == 'file':
                    r = requests.get(attachment['download_url'], stream=True)
                    with open(f"{attachment_dir}/{attachment['id']}-{attachment['filename']}", "wb") as f:
                        for chunk in r.iter_content(chunk_size=1024):
                            if chunk:
                                f.write(chunk)
                else:
                    print(attachment["type"])


if __name__ == "__main__":

    # Get the event number from cli args
    if len(os.sys.argv) > 1:
        EVENT_NUMBER = os.sys.argv[1]

    main()
