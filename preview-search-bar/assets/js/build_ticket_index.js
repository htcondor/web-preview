// Generate idx using command at root:
// echo "$(cat assets/data/search_documents/ticket_search.json)" | node assets/js/build_ticket_index.js > assets/data/search_indexes/ticket_index.json
//
// ticket_search.json contains the json of just space delimited text that occurs in the ticket organized by category
const fs = require('fs')
const lunr = require('lunr')



let parameters = {}
try {
    parameters = JSON.parse(fs.readFileSync(process.argv[2], 'utf8'))
} catch (err) {
    console.error(err)
}

let ref = parameters.ref
let fields = parameters.fields
let index_output = parameters.index_output

let documents = []
try {
    documents = JSON.parse(fs.readFileSync("assets/data/search_documents/ticket_search.json", 'utf8'))
} catch (err) {
    console.error(err)
}

let idx = lunr(function () {
    this.ref(ref)
    fields.forEach(function(field){
        if("boost" in field){
            this.field(field.id, field.boost)
        } else {
            this.field(field.id)
        }
    }, this)

    documents.forEach(function (doc) {
        this.add(doc)
    }, this)
})

fs.writeFile(index_output, JSON.stringify(idx), err => {
    if (err) {
        console.error(err)
        return
    }
    //file written successfully
})