const randomColor = () => {
    const r = Math.random() * 255;
    const g = Math.random() * 255;
    const b = Math.random() * 255;

    return `rgba(${r}, ${g}, ${b}, 0.2)`;
}

const deepCopy = source => {
    return JSON.parse(JSON.stringify(source))
}

// const data_path = "./data/benchmarks_sparse_matrix.json"
const data_path = "./data/benchmarks.json"

const response = await fetch(data_path);
const history = await response.json();
const benchmarks = history[0].benchmarks;

const createControl = (metadata, runs) => {
    const container = document.querySelector(".controls main");

    let history = "";
    for(let i = 0; i < runs.length; i++) {
        history += `<span><input name="history" type="checkbox" value="${i}" checked \><label>${i}</label></span>`
    }

    let dataSize = "";
    for(let i = 0; i < metadata.length; i++) {
        const size = metadata[i].size;
        dataSize += `<span><input name="size" type="checkbox" value="${i}" checked \><label>${size}</label></span>`
    }

    const names = []
    const layouts = ['Dense', 'Sparse'];
    let layoutContent = `<span><input name="layout" type="checkbox" value="${layouts[0]}" checked \><label>${layouts[0]}</label></span>`;
    layoutContent += `<span><input name="layout" type="checkbox" value="${layouts[1]}" checked \><label>${layouts[1]}</label></span>`;


    for(const benchmark of runs[0].benchmarks){
        let [$1, name, $2] = benchmark.name.split("/");

        for(const layout of layouts){
            const length = name.indexOf(layout);
            name = length !== -1 ? name.substring(0, length) : name;
        }
        if(!names.find(it => it === name)){
            names.push(name)
        }
    }

    let solvers = "";
    for(let i = 0; i < names.length; i++){
        const name = names[i];
        solvers += `<div><input name="solver" type="checkbox" value="${i}" checked \><label>${name}</label></div>`
    }


    container.innerHTML  =
`<form>
    <fieldset>
        <legend>history</legend>
        ${history}
    </fieldset>
    <fieldset>
        <legend>data size</legend>
        ${dataSize}
    </fieldset>
    <fieldset>
        <legend>layout</legend>
        ${layoutContent}
    </fieldset>
    <fieldset>
        <legend>solvers</legend>
        ${solvers}
    </fieldset>
</form>`

    container.querySelector('form').addEventListener('change', e => {
        const controls = Array.from(container.querySelectorAll(`input[type=checkbox]`));

        const staging = new Map();
        for(const control of controls){
            if(!staging.has(control.name)){
                staging.set(control.name, []);
            }
            staging.get(control.name).push(!control.checked)
            if(control.name === 'solver'){
                staging.get(control.name).push(!control.checked)
            }
        }
        const filter = {};
        for(const [key, value] of staging){
            filter[key] = value;
        }
        updateGraph(filter)
    });

}

const createChart = data => {
    document.querySelector('.graph').innerHTML = '';
    for(const entry of data) {
        const div = document.createElement("div");
        div.innerHTML = "<canvas width='500' height='300'></canvas>"
        document.querySelector('.graph').appendChild(div);

        new Chart(div.querySelector('canvas'), {
            type: 'bar',
            data: entry,
            options: {
                scales: {
                    y: {
                        beginAtZero: true
                    }
                }
            }
        });
    }
}
const updateGraph = filter => {
    if(!window.graph_data) return;

    const data = deepCopy(window.graph_data).filter((_, index) => !filter.size[index] );

    const layouts = ['Dense', 'Sparse'].filter((_, index) => filter.layout[index] );

    for(const entry of data){
        entry.datasets = entry.datasets.filter((_, index) => !filter.history[index] );

        entry.labels = entry.labels.filter((_, index) => !filter.solver[index] );
        for(const dataset of entry.datasets){
            dataset.data = dataset.data.filter((_, index) => !filter.solver[index]);
        }

        const indexes = entry.labels.map( (label, index) => {
            return !layouts.some(layout => label.includes(layout));
        });

        entry.labels = entry.labels.filter((_, index) => indexes[index]  );
        for(const dataset of entry.datasets){
            dataset.data = dataset.data.filter((_, index) => indexes[index] );
        }

    }
    createChart(data);
}


const constructGraph =  (metadata, history) => {
    const data = [];
    const hcolors =  [];

    for(let i = 0; i < history.length; i++){
        const colors = [];
        for (let j = 0; j < metadata.length; j++) {
            colors.push(randomColor());
        }
        hcolors.push(colors);
    }

    for(let { size, names, colors }  of metadata){
        data.push({
            labels: names,
            datasets: []
        });
        for(let i = 0; i < history.length; i++){
            data[data.length - 1].datasets.push(            {
                label: `Linear system solvers ${size} run(${i})`,
                data: [],
                borderWidth: 1,
                backgroundColor: hcolors[i]
            })
        }
    }

    for(let i = 0; i < history.length; i++) {
        for (const benchmark of history[i].benchmarks) {
            const [_, name, size] = benchmark.name.split("/");
            const index = metadata.findIndex(e => e.size == size);
            data[index].datasets[i].data.push(benchmark.real_time);
        }
    }

    createChart(data);

    window.graph_data = data;
}


const extractMetaData = benchmarks => {
    const set = new Set();

    const colors = [];
    for (const benchmark of benchmarks) {
        const [_, _1, size] = benchmark.name.split("/");
        set.add(size);

    }

    const sizes = [];
    for(const size of set){
        sizes.push(size);
        colors.push(randomColor());
    }

    const names = []
    for(const benchmark of benchmarks){
        const [_, name, size] = benchmark.name.split("/");
        if(size == sizes[0]){
            names.push(`${name} (${benchmark.time_unit})`);
        }
    }

    const metadata = [];

    for(const size of sizes){
        metadata.push({ size : Number(size), names, colors});
    }

    return metadata;
}

const metadata = extractMetaData(benchmarks);

createControl(metadata, history);
constructGraph(metadata, history);


