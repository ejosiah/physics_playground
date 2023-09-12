import createGraph from "./graph.js"

const layouts = ['Dense', 'Sparse'];
const layoutContent =
`<span><input name="layout" type="checkbox" value="${layouts[0]}" checked \><label>${layouts[0]}</label></span>
<span><input name="layout" type="checkbox" value="${layouts[1]}" checked \><label>${layouts[1]}</label></span>`;


const rename = names => {
    const newNames = [];
    names.forEach(name => {
       const layout = layouts.filter(layout => name.includes(layout));
        const length = name.indexOf(layout);
        name = (length !== -1) ? name.substring(0, length) : name;
        if(!newNames.find(it => it === name)){
            newNames.push(name)
        }
    });
    return newNames;
};

await createGraph("./data/sparse_vector.bencmarks.json"
    , {ame : "Sparse Vector"}
);