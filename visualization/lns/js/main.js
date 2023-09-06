const randomColor = () => {
    const r = Math.random() * 255;
    const g = Math.random() * 255;
    const b = Math.random() * 255;

    return `rgba(${r}, ${g}, ${b}, 0.2)`;
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
    for(let {size, $1, $2} of metadata) {
        dataSize += `<span><input name="size" type="checkbox" value="${size}" checked \><label>${size}</label></span>`
    }

    const names = []
    const layouts = ['Dense', 'Sparse'];
    let layoutContent = "";
    for(const layout of layouts){
        layoutContent += `<span><input name="layout" type="checkbox" value="${layout}" checked \><label>${layout}</label></span>`;
    }

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
    for(const name of names){
        solvers += `<div><input name="solver" type="checkbox" value="${name}" checked \><label>${name}</label></div>`
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
        const filter = [];
        const controls = Array.from(container.querySelectorAll(`input[name=${e.target.name}]`));
        controls.forEach(control => filter.push(!control.checked));
        const options = { [e.target.name] : filter };
        constructGraph(metadata, runs, options);
    })

}

const constructGraph =  (metadata, history, options = {}) => {
    document.querySelector('.graph').innerHTML = '';
    const data = [];
    const hcolors = [];
// const alldatasets = [];
    for(let i = 0; i < history.length; i++){
        const colors = [];
        for(let j = 0; j < metadata.length; j++){
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

