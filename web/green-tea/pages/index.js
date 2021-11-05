import Head from "next/head";
import Dashboard from "../components/Dashboard";
import Sidebar from "../components/Sidebar";

export default function Home() {
    return (
        <div className="flex">
            <Head>
                <title>GreenTea Dashboard</title>
                <link rel="icon" href="/greentea.ico" />
            </Head>
            <Sidebar />
            <Dashboard />
        </div>
    );
}
