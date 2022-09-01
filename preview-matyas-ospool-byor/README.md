# HTCondor Website

This git repository contains the source code for the refactored HTCondor website.If you want to learn more about HTCondor, [visit the website itself](https://htcondor.org).

- [Development](#development)
- [Creating Preview Branches](#previewing-branches)
- [Making Website Changes](#making-website-changes)


# Development

The HTCondor website is implemented in [Jekyll](https://jekyllrb.com/). We've provided a Docker image which contains all the necessary libraries to build the website and preview it in a web browser.

## Obtaining Docker image

Grab the **htcondor/web** image from our Docker Hub:
```
docker pull dockerreg.chtc.wisc.edu:443/htcondor/web
```

## Building the website

**Make sure you have pulled submodules before building, otherwise you will be missing files and your build will break.**

After making changes to the Jekyll source files, use the Docker image to preview your changes. Run the following from your computer while inside the checked-out copy of the website source:

```
docker run -p 8000:8000 --rm --volume $PWD:/srv/jekyll:Z -it dockerreg.chtc.wisc.edu:443/htcondor/web
```

This will utilize the latest Jekyll version and map port `8000` to your host.  Within the container, a small HTTP server can be started with the following command:

```
jekyll serve --watch -H 0.0.0.0 -P 8000
```

This will build and serve the website; it can be viewed by navigating your web browser to <http://localhost:8000>.

With the `--watch` flag set, any changes you make to the website source will cause a new version of the website to be built; it usually takes 4-5 seconds between hitting "Save" and then "Refresh" on the website.


## Previewing Branches

If you would like to preview your branch or test it with the website production build you have the option now. 

To create a preview branch:
1. Prepend the name of your branch with "preview-"
    - Example: "preview-helloworld"
2. Push this branch to https://github.com/htcondor/htcondor-web.git  
3. Check for your preview branch @ https://htcondor.com/web-preview/<preview-branch-name>
   - Example: [https://htcondor.com/web-preview/preview-helloworld/](https://htcondor.com/web-preview/preview-helloworld/)
4. When you merge in the branch or no longer need it, delete the branch and Github will delete the web preview   
   
This is a great way to demo your changes to others easily. 

## Making Website Changes

:exclamation: **New workflow is live making master the production branch.**

1.  Submit a pull request with website updates to the `master` branch (the default) and request a review.
2.  If the PR is approved you can merge into master.
3.  Check that your intended changes populate at [htcondor.org](https://htcondor.org)
