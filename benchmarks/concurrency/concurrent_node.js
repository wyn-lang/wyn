function compute() {
    let sum = 0;
    for (let i = 0; i < 1000000; i++) {
        sum += i * i;
    }
}

async function main() {
    const promises = [];
    for (let i = 0; i < 10; i++) {
        promises.push(Promise.resolve().then(() => compute()));
    }
    await Promise.all(promises);
}

main();
