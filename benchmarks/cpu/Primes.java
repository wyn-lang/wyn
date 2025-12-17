public class Primes {
    static boolean isPrime(int n) {
        if (n < 2) return false;
        if (n == 2) return true;
        if (n % 2 == 0) return false;
        
        for (int i = 3; i * i <= n; i += 2) {
            if (n % i == 0) return false;
        }
        return true;
    }
    
    public static void main(String[] args) {
        int count = 0;
        for (int n = 2; n < 100000; n++) {
            if (isPrime(n)) count++;
        }
        System.out.println(count);
    }
}
