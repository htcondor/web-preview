# HTCondor Website

This git repository contains the source code for the refactored HTCondor website.If you want to learn more about HTCondor, [visit the website itself](https://htcondor.org).


# Development

The HTCondor website is implemented in [Jekyll](https://jekyllrb.com/). We've provided a Docker image which contains all the necessary libraries to build the website and preview it in a web browser.

## Obtaining Docker image

Grab the **htcondor/web** image from our Docker Hub:
```
docker pull dockerreg.chtc.wisc.edu:443/htcondor/web
```

## Building the website

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


# Making Website Changes

:exclamation: **New workflow will be implemented soon based on dev comments.**
Email will go out and README will be updated when these changes go live

This repository has to ability to use [GitHub Actions](https://github.com/htcondor/htcondor-web/blob/master/.github/workflows/)
to deploy a website preview from the `master` branch to the [web-preview repository](https://htcondor.github.io/web-preview/).

To use this feature prepend your branch name with "preview-" then push that branch to htcondor-web. 
You will now find your new branch on the web-preview page under the url "https://htcondor.github.io/web-preview/preview-...".

For instance if you push "preview-helloworld", you will find your dev preview at "https://htcondor.github.io/web-preview/preview-helloworld"

The production websites (htcondor.com, https://research.cs.wisc.edu/htcondor/) are built automatically by GitHub Pages from the `production` branch.

To make changes to the website, use the following workflow:

1.  Submit a pull request with website updates to the `master` branch (the default) and request a review
2.  Upon approval and merge of the pull request, changes can be previewed at https://htcondor.github.io/
3.  If additional changes are necessary, repeat steps 1 and 2.
4.  When satisfied with the preview website, submit a
    [pull request](https://github.com/htcondor/htcondor-web/compare/production...master)
    from `production` to `master`
5.  After the pull request from step 4 has been merged, verify the changes at https://research.cs.wisc.edu/htcondor/
