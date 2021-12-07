let SoftwareDisplay = function(){
    this.software_headers = document.getElementsByClassName("accordion-header")
    this.software_briefs = document.getElementsByClassName("desktop-software-brief")

    for (const software_brief of this.software_briefs) {
        if( document.getElementById(software_brief.id).classList.contains("open") ){
            this.open_id = software_brief.id.replace("-desktop-brief", "")
        }
    }

    this.close_brief = function(brief_id){
        document.getElementById(brief_id).classList.remove("open")
    }

    this.open_brief = function(brief_id){
        document.getElementById(brief_id).classList.add("open")
    }

    this.toggle_brief = async function(event){
        let toggled_id = event.currentTarget.id.replace("-header", "")
        let toggled_brief_id = toggled_id + "-desktop-brief";
        let open_brief_id = this.open_id + "-desktop-brief";

        // If a brief is open then close it
        if( this.open_id ){
            this.close_brief(open_brief_id);
        }

        // If the toggled option is already open we only have to close a brief
        if( this.open_id != toggled_id ) {

            // If a brief was open then wait until it is closed to open one
            if( this.open_id ){
                setTimeout(() => this.open_brief(toggled_brief_id), 300);

            } else {
                this.open_brief(toggled_brief_id)

            }
            this.open_id = toggled_id

        } else {

            this.open_id = false;
        }
    }

    for (const software_header of this.software_headers) {
        software_header.addEventListener("click", (event) => this.toggle_brief(event))
    }
}

const software_display = new SoftwareDisplay()