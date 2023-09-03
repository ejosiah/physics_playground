const ctx = document.getElementById('myChart');

const response = await fetch("./data/benchmarks.json");
const history = await response.json();
const benchmarks = history[0].benchmarks;

const randomColor = () => {
    const r = Math.random() * 255;
    const g = Math.random() * 255;
    const b = Math.random() * 255;

    return `rgba(${r}, ${g}, ${b}, 0.2)`;
}

const data = {
    labels: [],
    datasets: [
        {
            label: "Linear system solvers",
            data: [],
            borderWidth: 1,
            backgroundColor: []
        }
    ]
};
for (const benchmark of benchmarks) {
    const label = benchmark.name
    data.labels.push(label);
    data.datasets[0].data.push(Math.log(benchmark.cpu_time));
    data.datasets[0].backgroundColor.push(randomColor());
}


new Chart(ctx, {
    type: 'bar',
    data: data,
    options: {
        scales: {
            y: {
                beginAtZero: true
            }
        }
    }
});
