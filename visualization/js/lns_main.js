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

await createGraph("./data/linear_solvers_benchmarks.json"
    , {
                name : "Linear system solvers",
                rename,
                extraControls : [{position: 2, component : { layout : layoutContent}}],
                update: (data, filter) => {
                    const alayout = layouts.filter((_, index) => filter.layout[index] );

                    const indexes = data.labels.map( label => {
                        return !alayout.some(alayout => label.includes(alayout));
                    });

                    data.labels = data.labels.filter((_, index) => indexes[index]  );
                    for(const dataset of data.datasets){
                        dataset.data = dataset.data.filter((_, index) => indexes[index] );
                    }
                } }
);