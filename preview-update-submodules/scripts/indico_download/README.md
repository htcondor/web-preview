# Update the number (2175) with the event id found in the indico page url. 
# For example HTC2024 url: https://agenda.hep.wisc.edu/event/2175/

This script generates the summary report to be placed in the HTCondor past workshop summary page. To create
a new page update the event number below found in the Indico page url and run the script. The script will also download
all associated attachments.

The script will generate a yaml file that can be copied and pasted into the `_event_summaries` folder. Rename the file
in accordance to the other files there, add a title, and convert the yaml to markdown frontmatter.

```bash
docker build -t indico_download . && docker run --rm -it -v $(PWD):/app indico_download 2175
```
fetch
