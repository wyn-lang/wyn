public class concurrent_java {
    static void compute() {
        long sum = 0;
        for (int i = 0; i < 1000000; i++) {
            sum += i * i;
        }
    }
    
    public static void main(String[] args) throws InterruptedException {
        Thread[] threads = new Thread[10];
        for (int i = 0; i < 10; i++) {
            threads[i] = new Thread(() -> compute());
            threads[i].start();
        }
        for (Thread t : threads) {
            t.join();
        }
    }
}
